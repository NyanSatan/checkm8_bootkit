#ifndef libbootkit_H
#define libbootkit_H

#include <stdbool.h>
#include <sys/types.h>
#include <libirecovery.h>
#include "config.h"
#include "config_watch.h"

const config_t *get_config(uint32_t cpid);
const config_watch_t *get_config_watch(uint32_t cpid);

int dfu_boot(irecv_client_t client, const char *bootloader, size_t bootloader_length, bool debug);
int dfu_boot_watch(irecv_client_t client, const char *bootloader, size_t bootloader_length, bool debug);

#endif
