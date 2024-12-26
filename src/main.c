#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "libbootkit/libbootkit.h"

int str2hex(size_t buflen, uint8_t *buf, const char *str) {
    unsigned char *ptr = buf;
    int seq = -1;
    while (buflen > 0) {
        int nibble = *str++;
        if (nibble >= '0' && nibble <= '9') {
            nibble -= '0';
        } else {
            nibble |= 0x20;
            if (nibble < 'a' || nibble > 'f') {
                break;
            }
            nibble -= 'a' - 10;
        }
        if (seq >= 0) {
            *buf++ = (seq << 4) | nibble;
            buflen--;
            seq = -1;
        } else {
            seq = nibble;
        }
    }

    return buf - ptr;
}

void print_usb_serial(irecv_client_t client) {
    const struct irecv_device_info *info = irecv_get_device_info(client);
    printf("found: %s\n", info->serial_string);
}

void print_usage(const char *program_name) {
    printf("usage: %s VERB [args]\n", program_name);
    printf("where VERB is one of the following:\n");
    printf("\tboot <bootloader>\n");
    printf("\tkbag <kbag>\n");
    printf("\tdemote\n");
    printf("\tundemote\n");

    printf("\nsupported platforms:\n\t");

    size_t number_of_configs = sizeof(rom_configs) / sizeof(rom_config_t);

    for (int i = 0; i < number_of_configs; i++) {
        const rom_config_t *config = &rom_configs[i];
        (i != number_of_configs - 1) ? printf("%s, ", config->platform) : printf("%s", config->platform);
    }

    printf("\n");
}

enum {
    VERB_NONE = -1,
    VERB_BOOT,
    VERB_KBAG,
    VERB_DEMOTE,
    VERB_UNDEMOTE
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        goto usage;
    }

    int ret = -1;
    int verb = VERB_NONE;
    bool debug = false;

    int fd = -1;
    uint8_t *bootloader_buffer = NULL;
    size_t bootloader_size = 0;

    uint8_t kbag[KBAG_LEN_256] = { 0 };
    size_t kbag_len = 0;

    if (strcmp(argv[1], "boot") == 0) {
        verb = VERB_BOOT;

        if (argc != 3) {
            goto missing_arg;
        }

        const char *bootloader = argv[2];

        fd = open(bootloader, O_RDONLY);
        if (fd < 0) {
            printf("failed to open bootloader\n");
            goto out;
        }

        bootloader_size = lseek(fd, 0, SEEK_END);
        if (!bootloader_size) {
            printf("bootloader is empty?!\n");
            goto out;
        }

        bootloader_buffer = malloc(bootloader_size);
        if (!bootloader_buffer) {
            printf("out of memory\n");
            goto out;
        }

        if (pread(fd, bootloader_buffer, bootloader_size, 0) != bootloader_size) {
            printf("failed to read bootloader\n");
            goto out;
        }

        close(fd);
        fd = 0;

    } else if (strcmp(argv[1], "kbag") == 0) {
        verb = VERB_KBAG;

        if (argc != 3) {
            goto missing_arg;
        }

        const char *raw_kbag = argv[2];

        size_t raw_kbag_len = strlen(raw_kbag);

        if (raw_kbag_len == KBAG_LEN_256 * 2) {
            kbag_len = KBAG_LEN_256;
        } else if (raw_kbag_len == KBAG_LEN_128 * 2) {
            kbag_len = KBAG_LEN_128;
        } else {
            printf("KBAG must be %d bytes in length (or %d bytes for Haywire)\n", KBAG_LEN_256, KBAG_LEN_128);
            return -1;
        }

        if (str2hex(kbag_len, kbag, raw_kbag) != kbag_len) {
            printf("failed to decode KBAG\n");
            return -1;
        }

    } else if (strcmp(argv[1], "demote") == 0) {
        verb = VERB_DEMOTE;
    } else if (strcmp(argv[1], "undemote") == 0) {
        verb = VERB_UNDEMOTE;
    }

    if (verb == VERB_NONE) {
        goto usage;
    }

    irecv_client_t client = NULL;
    if (irecv_open_with_ecid(&client, 0) != IRECV_E_SUCCESS) {
        printf("ERROR: failed to open DFU-device\n");
        return -1;
    }

    print_usb_serial(client);

    const struct irecv_device_info *info = irecv_get_device_info(client);
    const rom_config_t *config = get_config(info->cpid);

    switch (verb) {
        case VERB_BOOT: {
            printf("loading iBoot...\n");

            if (!config->watch) {
                ret = dfu_boot(client, bootloader_buffer, bootloader_size, debug);
            } else {
                ret = dfu_boot_watch(client, bootloader_buffer, bootloader_size, debug);
            }

            goto boot_out;
        }

        case VERB_KBAG: {
            printf("decrypting KBAG...\n");

            if (kbag_len == KBAG_LEN_128 && config->cpid != 0x8747) {
                printf("128-bit KBAG provided for non-Haywire target\n");
                return -1;
            }

#define AES_IV_SIZE 0x10

            ret = aes_op(client, config, kbag, kbag_len);
            if (ret == 0) {
                printf("iv: ");
                for (size_t i = 0; i < AES_IV_SIZE; i++) {
                    printf("%02X", kbag[i]);
                }

                printf(" ");

                printf("key: ");
                for (size_t i = AES_IV_SIZE; i < kbag_len; i++) {
                    printf("%02X", kbag[i]);
                }

                printf("\n");
            }

            break;
        }

        case VERB_DEMOTE: {
            printf("demoting...\n");
            ret = demote_op(client, config, true);
            break;
        }

        case VERB_UNDEMOTE: {
            printf("undemoting...\n");
            ret = demote_op(client, config, false);
            break;
        }
    }

    goto out;

boot_out:
    if (fd != -1) {
        close(fd);
    }

    if (bootloader_buffer) {
        free(bootloader_buffer);
    }

out:
    if (client) {
        irecv_close(client);
    }

    if (ret == 0) {
        printf("DONE\n");
    }

    return ret;

missing_arg:
    printf("\"%s\" needs argument!\n", argv[1]);

usage:
    print_usage(argv[0]);
    return -1;
}
