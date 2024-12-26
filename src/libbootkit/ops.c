#include <stdio.h>
#include <lilirecovery.h>
#include "ops.h"
#include "protocol.h"
#include "config.h"

int demote_op(irecv_client_t client, const rom_config_t *config, bool demote) {
    if (!config->demotion_reg) {
        printf("%s demotion is not supported\n", config->platform);
        return -1;
    }

    uint32_t value = 0;
    int ret = read32(client, config, config->demotion_reg, &value);
    if (ret != 0) {
        printf("couldn't read demotion register\n");
        return -1;
    }

    printf("curr: 0x%08x\n", value);

    if (demote) {
        if ((value & 0x1) == 0x0) {
            printf("already demoted\n");
            return -1;
        }

        value &= ~0x1;
    } else {
        value |= 0x1;
    }
    
    printf("new:  0x%08x\n", value);

    ret = write32(client, config, config->demotion_reg, value);
    if (ret != 0) {
        printf("couldn't write demotion register\n");
        return -1;
    }

    return 0;
}

#define AES_OP_DECRYPT  0x11
#define AES_KEY_GID0    0x200
#define AES_256         0x20000000

int aes_op(irecv_client_t client, const rom_config_t *config, uint8_t kbag[KBAG_LEN_256], size_t len) {
    struct {
        uint32_t op;
        uint32_t in;
        uint32_t out;
        uint32_t len;
        uint32_t key;
        uint32_t user_key;
        uint32_t user_iv;
    } args = { 0 };

    args.op = AES_OP_DECRYPT;
    args.in = config->loadaddr + AUX_DATA_START;
    args.out = config->loadaddr + sizeof(usb_command_done_t);
    args.len = len;
    args.key = AES_KEY_GID0 | (len == KBAG_LEN_256 ? AES_256 : 0x0);

    return execute(client, config, kbag, len, config->aes_crypto_cmd, (uint32_t *)&args, kbag, len);
}
