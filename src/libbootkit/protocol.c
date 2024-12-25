#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <lilirecovery.h>

#include "config.h"
#include "dfu.h"
#include "protocol.h"

int validate_device(irecv_client_t client) {
    const struct irecv_device_info *info = irecv_get_device_info(client);

    int mode;

    if (irecv_get_mode(client, &mode) != IRECV_E_SUCCESS) {
        printf("failed to get device mode\n");
        return -1;
    }

    if (mode != IRECV_K_DFU_MODE) {
        printf("non-DFU device found\n");
        return -1;
    }

    if (!info->srtg) {
        printf("soft-DFU device found\n");
        return -1;
    }

    if (!strstr(info->serial_string, "PWND:[checkm8]")) {
        printf("non-pwned-DFU device found\n");
        return -1;
    }

    return 0;
}

int send_command(irecv_client_t client, unsigned char *command, size_t length) {
    printf("sending command...\n");

    if (request_image_validation(client, false) != 0) {
        printf("failed to prepare command sending\n");
        return -1;
    }

    if (send_data(client, command, length) != 0) {
        printf("failed to send command buffer\n");
        return -1;        
    }

    return 0;
}

int trigger_command(irecv_client_t client, unsigned char *response, size_t response_length) {
    if (!response_length) {
        uint8_t dummy_data;
        return irecv_usb_control_transfer(client, 0xA1, 2, 0xFFFF, 0, (unsigned char*)&dummy_data, sizeof(dummy_data), USB_TIMEOUT);
    } else {
        return irecv_usb_control_transfer(client, 0xA1, 2, 0xFFFF, 0, (unsigned char*)response, response_length, USB_TIMEOUT);
    }
}

int save_command(irecv_client_t client, unsigned char *command, size_t length) {
    const struct irecv_device_info *info = irecv_get_device_info(client);

    char path[40];
    snprintf((char *)&path, sizeof(path), "/tmp/%04X-%016llX_%08X", info->cpid, info->ecid, arc4random_uniform(UINT32_MAX));

    int fd = open(path, O_WRONLY | O_CREAT, 0644);

    if (fd < 0) {
        printf("failed to create output file\n");
        return -1;
    }

    if (write(fd, command, length) != length) {
        printf("failed to write to output file\n");
        return -1;
    }

    printf("written to %s\n", path);

    return 0;
}

int read32(irecv_client_t client, const rom_config_t *config, uint32_t address, uint32_t *dest) {
    printf("reading 32-bits from 0x%x...\n", address);

    struct {
        usb_command_memc_t header;
    } cmd = { 0 };

    cmd.header.magic  = USB_COMMAND_MEM_MAGIC;
    cmd.header.magic2 = USB_COMMAND_MEM_MAGIC;
    cmd.header.dest_ptr = config->loadaddr + sizeof(usb_command_done_t);
    cmd.header.src_ptr = address;
    cmd.header.length = sizeof(uint32_t);

    if (send_command(client, (unsigned char*)&cmd, sizeof(cmd)) != 0) {
        printf("failed to send command\n");
        return -1;
    }

    struct {
        usb_command_done_t header;
        uint32_t data;
    } resp = { 0 };

    if (trigger_command(client, (unsigned char*)&resp, sizeof(resp)) != sizeof(resp)) {
        printf("failed to receive data\n");
        return -1;
    }

    if (resp.header.magic != USB_COMMAND_DONE_MAGIC || resp.header.magic2 != USB_COMMAND_DONE_MAGIC) {
        printf("invalid response from device\n");
        return -1;
    }

    *dest = resp.data;

    return 0;
}

int write32(irecv_client_t client, const rom_config_t *config, uint32_t address, uint32_t value) {
    printf("writing 0x%x to 0x%x...\n", value, address);

    struct {
        usb_command_memc_t header;
        uint32_t data;
    } cmd = { 0 };

    cmd.header.magic  = USB_COMMAND_MEM_MAGIC;
    cmd.header.magic2 = USB_COMMAND_MEM_MAGIC;
    cmd.header.dest_ptr = address;
    cmd.header.src_ptr = config->loadaddr + sizeof(usb_command_memc_t);
    cmd.header.length = sizeof(uint32_t);
    cmd.data = value;

    if (send_command(client, (unsigned char*)&cmd, sizeof(cmd)) != 0) {
        printf("failed to send command\n");
        return -1;
    }
    
    trigger_command(client, NULL, 0);

    uint32_t verify = 0;

    if (read32(client, config, address, &verify) != 0) {
        printf("failed to re-read value\n");
        return -1;
    }

    if (verify != value) {
        printf("re-read value doesn't match with requested one\n");
        return -1;
    }

    return 0;
}

int execute(irecv_client_t client, const rom_config_t *config, uint8_t *output, size_t output_len, uint32_t address, uint32_t args[MAX_ARGS], uint8_t *aux_data, size_t aux_data_len) {
    struct {
        usb_command_t header;
        uint32_t args[MAX_ARGS];
    } cmd = { 0 };

    cmd.header.magic = USB_COMMAND_EXEC_MAGIC;
    cmd.header.magic2 = USB_COMMAND_EXEC_MAGIC;
    cmd.header.function = address;

    memcpy(cmd.args, args, sizeof(cmd.args));

    if (send_command(client, (uint8_t *)&cmd, sizeof(cmd)) != 0) {
        printf("failed to send execute command\n");
        return -1;
    }

    if (aux_data) {
        if (send_data(client, aux_data, aux_data_len) != 0) {
            printf("failed to send command auxilary data\n");
            return -1;
        }
    }

    struct {
        usb_command_done_t header;
        uint8_t buffer[0x30];
    } resp = { 0 };

    trigger_command(client, (uint8_t *)&resp, sizeof(usb_command_done_t) + output_len);

    if (resp.header.magic != USB_COMMAND_DONE_MAGIC || resp.header.magic2 != USB_COMMAND_DONE_MAGIC) {
        printf("invalid response from device\n");
        return -1;
    }

    memcpy(output, resp.buffer, output_len);

    return 0;
}
