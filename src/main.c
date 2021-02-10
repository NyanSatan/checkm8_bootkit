#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "libbootkit/libbootkit.h"
#include "libbootkit/config.h"

void print_confidential_text() {
    printf("THIS IS CONFIDENTIAL AND PROPRIETARY SOFTWARE FROM JOHN (@NYAN_SATAN)\n");
    printf("IF YOU ACQUIRED THIS SOFTWARE WITHOUT HIS PERMISSION YOU HAVE TO\n");
    printf("                     IMMEDIATELY COMMIT SUICIDE                      \n");
    printf("\n");
}

void print_usb_serial(irecv_client_t client) {
    const struct irecv_device_info *info = irecv_get_device_info(client);
    printf("found: %s\n", info->serial_string);
}

void print_usage(const char *program_name) {
    printf("usage: %s <bootloader> [--debug]\n", program_name);
    printf("supported platforms: ");

    size_t number_of_configs = sizeof(configs) / sizeof(config_t);

    for (int i = 0; i < number_of_configs; i++) {
        const config_t *config = &configs[i];
        (i != number_of_configs - 1) ? printf("%s, ", config->platform) : printf("%s", config->platform);
    }

    printf("\n");
}

int main(int argc, char *argv[]) {
    //print_confidential_text();

    if (argc < 2) {
        goto usage;
    }

    bool debug = false;

    if (argc == 3) {
        if (strcmp(argv[2], "--debug") == 0)
            debug = true;
        else
            goto usage;
    }

    const char *bootloader = argv[1];

    int fd = open(bootloader, O_RDONLY);
    if (fd < 0) {
        printf("ERROR: failed to open input file\n");
        return -1;
    }

    long size = lseek(fd, 0, SEEK_END);
    if (!size) {
        printf("ERROR: file is empty\n");
        return -1;
    }

    uint8_t *buffer = malloc(size);
    if (!buffer) {
        printf("ERROR: out of memory\n");
        return -1;
    }

    if (pread(fd, buffer, size, 0) < 0) {
        printf("ERROR: failed to read input file\n");
        return -1;
    }

    irecv_client_t client;
    if (irecv_open_with_ecid(&client, 0) != IRECV_E_SUCCESS) {
        printf("ERROR: failed to open DFU-device\n");
        return -1;
    }

    print_usb_serial(client);

    int ret = dfu_boot(client, (const char*)buffer, size, debug);

    if (ret == 0)
        printf("DONE\n");

    return ret;

usage:
    print_usage(argv[0]);
    return -1;
}
