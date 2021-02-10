#ifndef libbootkit_H
#define libbootkit_H

#include <stdbool.h>
#include <sys/types.h>
#include <lib/libirecovery.h>

int dfu_boot(irecv_client_t client, const char *bootloader, size_t bootloader_length, bool debug);

#endif
