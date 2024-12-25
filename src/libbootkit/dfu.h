#ifndef dfu_H
#define dfu_H

#include <stdbool.h>
#include <lilirecovery.h>

#define MAX_PACKET_SIZE     0x800
#define USB_SMALL_TIMEOUT   100
#define USB_TIMEOUT         5000

int send_data(irecv_client_t client, unsigned char *command, size_t length);
int request_image_validation(irecv_client_t client, bool needs_reset);

#endif
