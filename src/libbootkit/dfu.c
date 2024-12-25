#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "dfu.h"

static size_t min(size_t first, size_t second) {
    if (first < second) {
        return first;
    } else {
        return second;
    }
}

int send_data(irecv_client_t client, unsigned char *command, size_t length) {
    size_t index = 0;

    while (index < length) {
        size_t amount = min(length - index, MAX_PACKET_SIZE);
        if (irecv_usb_control_transfer(client, 0x21, 1, 0, 0, command + index, amount, USB_TIMEOUT) != amount) {
            return -1;
        }
        index += amount;
    }

    return 0;
}

int request_image_validation(irecv_client_t client, bool needs_reset) {
    unsigned char dummy_data[6];
    memset(&dummy_data, 0x0, sizeof(dummy_data));

    if (send_data(client, (unsigned char*)&dummy_data, sizeof(dummy_data)) != 0) {
        printf("failed to send dummy data\n");
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
