#ifndef ops_H
#define ops_H

#include <stdbool.h>
#include <lilirecovery.h>
#include "config.h"

int demote_op(irecv_client_t client, const rom_config_t *config, bool demote);

#define KBAG_LEN_256    0x30
#define KBAG_LEN_128    0x20

int aes_op(irecv_client_t client, const rom_config_t *config, uint8_t kbag[KBAG_LEN_256], size_t len);

#endif
