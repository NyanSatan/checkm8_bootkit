#ifndef boot_H
#define boot_H

#include <stdbool.h>
#include <sys/types.h>
#include <lilirecovery.h>

int dfu_boot(irecv_client_t client, const uint8_t *bootloader, size_t bootloader_length, bool debug);
int dfu_boot_watch(irecv_client_t client, const uint8_t *bootloader, size_t bootloader_length, bool debug);

#endif
