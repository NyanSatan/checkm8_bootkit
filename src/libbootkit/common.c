#define MAX_PACKET_SIZE     0x800
#define USB_SMALL_TIMEOUT   100
#define USB_TIMEOUT         5000

#define ARM_RESET_VECTOR 0xEA00000E

static size_t min(size_t first, size_t second) {
    if (first < second)
        return first;
    else
        return second;
}

static int send_data(irecv_client_t client, unsigned char *command, size_t length) {
    size_t index = 0;

    while (index < length) {
        size_t amount = min(length - index, MAX_PACKET_SIZE);
        if (irecv_usb_control_transfer(client, 0x21, 1, 0, 0, command + index, amount, USB_TIMEOUT) != amount)
            return -1;
        index += amount;
    }

    return 0;
}

static int request_image_validation(irecv_client_t client, bool needs_reset) {
    unsigned char dummy_data[6];
    memset(&dummy_data, 0x0, sizeof(dummy_data));

    if (send_data(client, (unsigned char*)&dummy_data, sizeof(dummy_data)) != 0) {
        printf("ERROR: failed to send dummy data\n");
        return -1;        
    }

    irecv_usb_control_transfer(client, 0x21, 1, 0, 0, NULL, 0, USB_SMALL_TIMEOUT);
    irecv_usb_control_transfer(client, 0xA1, 3, 0, 0, (unsigned char*)&dummy_data, sizeof(dummy_data), USB_SMALL_TIMEOUT);
    irecv_usb_control_transfer(client, 0xA1, 3, 0, 0, (unsigned char*)&dummy_data, sizeof(dummy_data), USB_SMALL_TIMEOUT);

    if (needs_reset) {
        irecv_reset(client);
    }

    return 0;
}

static int send_command(irecv_client_t client, unsigned char *command, size_t length) {
    printf("sending command...\n");

    if (request_image_validation(client, false) != 0) {
        printf("ERROR: failed to prepare command sending\n");
        return -1;
    }

    if (send_data(client, command, length) != 0) {
        printf("ERROR: failed to send command buffer\n");
        return -1;        
    }

    return 0;
}

static int trigger_command(irecv_client_t client, unsigned char *response, size_t response_length) {
    if (!response_length) {
        uint8_t dummy_data;
        return irecv_usb_control_transfer(client, 0xA1, 2, 0xFFFF, 0, (unsigned char*)&dummy_data, sizeof(dummy_data), USB_TIMEOUT);
    } else {
        return irecv_usb_control_transfer(client, 0xA1, 2, 0xFFFF, 0, (unsigned char*)response, response_length, USB_TIMEOUT);
    }
}

static int validate_device(irecv_client_t client) {
    const struct irecv_device_info *info = irecv_get_device_info(client);

    int mode;

    if (irecv_get_mode(client, &mode) != IRECV_E_SUCCESS) {
        printf("ERROR: failed to get device mode\n");
        return -1;
    }

    if (mode != IRECV_K_DFU_MODE) {
        printf("ERROR: non-DFU device found\n");
        return -1;
    }

    if (!info->srtg) {
        printf("ERROR: soft-DFU device found\n");
        return -1;
    }

    if (!strstr(info->serial_string, "PWND:[checkm8]")) {
        printf("ERROR: non-pwned-DFU device found\n");
        return -1;
    }

    return 0;
}

static int save_command(irecv_client_t client, unsigned char *command, size_t length) {
    const struct irecv_device_info *info = irecv_get_device_info(client);

    char path[40];
    snprintf((char *)&path, sizeof(path), "/tmp/%04X-%016llX_%08X", info->cpid, info->ecid, arc4random_uniform(UINT32_MAX));

    int fd = open(path, O_WRONLY | O_CREAT, 0644);

    if (fd < 0) {
        printf("ERROR: failed to create output file\n");
        return -1;
    }

    if (write(fd, command, length) != length) {
        printf("ERROR: failed to write to output file\n");
        return -1;
    }

    printf("written to %s\n", path);

    return 0;
}
