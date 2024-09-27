#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "libbootkit.h"
#include "payload.h"
#include "config.h"
#include "common.c" // I am the bad guy

typedef struct {
    uint32_t magic;
    uint32_t magic2;
#define USB_COMMAND_MAGIC 'exec'
    uint32_t function;
    uint8_t padding[4];
} usb_command_t;

typedef struct __attribute__((packed)) {
    uint32_t loadaddr;
    uint32_t imageaddr;
    uint32_t imagesize;
    uint32_t memmove;
    uint32_t platform_get_boot_trampoline;
    uint32_t platform_bootprep;
    uint32_t usb_quiesce_no_free;
    uint32_t interrupt_mask_all;
    uint32_t timer_stop_all;
    uint32_t clocks_quiesce;
    uint32_t enter_critical_section;
    uint32_t arch_cpu_quiesce;
} payload_offsets_t;

const config_t *get_config(uint32_t cpid) {
    for (int i = 0; i < sizeof(configs) / sizeof(config_t); i++) {
        const config_t *config = &configs[i];
        if (config->cpid == cpid)
            return config;
    }

    return NULL;
}

static unsigned char *construct_payload(const config_t *config, off_t bootloader_offset, size_t bootloader_length) {
    printf("constructing payload...\n");
    
    unsigned char *payload_copy = malloc(sizeof(payload));
    if (!payload_copy) {
        printf("ERROR: out of memory\n");
        return NULL;
    }

    memmove(payload_copy, &payload, sizeof(payload));

    static const uint32_t magic = 0xDEAD0001;
    
    unsigned char *offset = memmem(payload_copy, sizeof(payload), &magic, sizeof(magic));
    if (!offset) {
        printf("ERROR: improper payload\n");
        return NULL;
    }

    if (sizeof(payload) - (size_t)(offset - payload_copy) < sizeof(payload_offsets_t)) {
        printf("ERROR: improper payload\n");
        return NULL;
    }

    payload_offsets_t *payload_offsets = (payload_offsets_t *)offset;

    payload_offsets->loadaddr = config->loadaddr;
    payload_offsets->imageaddr = config->loadaddr + bootloader_offset;
    payload_offsets->imagesize = bootloader_length;
    payload_offsets->memmove = config->memmove;
    payload_offsets->platform_get_boot_trampoline = config->platform_get_boot_trampoline;
    payload_offsets->platform_bootprep = config->platform_bootprep;
    payload_offsets->usb_quiesce_no_free = config->usb_quiesce_no_free;
    payload_offsets->interrupt_mask_all = config->interrupt_mask_all;
    payload_offsets->timer_stop_all = config->timer_stop_all;
    payload_offsets->clocks_quiesce = config->clocks_quiesce;
    payload_offsets->enter_critical_section = config->enter_critical_section;
    payload_offsets->arch_cpu_quiesce = config->arch_cpu_quiesce;

    return payload_copy;
}

static int construct_command(irecv_client_t client,
                      const char *bootloader,
                      size_t bootloader_length,
                      unsigned char **result,
                      size_t *result_length) {

    printf("constructing command...\n");

    if (*(uint32_t*)bootloader != ARM_RESET_VECTOR) {
        printf("ERROR: provided bootloader doesn't seem to be an ARM image\n");
        return -1;
    }

    const struct irecv_device_info *info = irecv_get_device_info(client);

    const config_t *config = get_config(info->cpid);
    if (!config) {
        printf("ERROR: no config available for CPID:%04X\n", info->cpid);
        return -1;
    }

    size_t command_length = sizeof(payload) + bootloader_length + sizeof(usb_command_t);

    if (command_length > config->loadsize) {
        printf("ERROR: resulting command is too big, use smaller bootloader (at least %lu smaller)\n", command_length - config->loadsize);
        return -1;
    }

    unsigned char *buffer = malloc(command_length);
    if (!buffer) {
        printf("ERROR: out of memory");
        return -1;
    }

    memset(buffer, 0x0, command_length);

    off_t bootloader_offset = sizeof(usb_command_t);
    off_t payload_offset = bootloader_offset + bootloader_length;

    usb_command_t *command = (usb_command_t *)buffer;
    command->magic = USB_COMMAND_MAGIC;
    command->magic2 = USB_COMMAND_MAGIC;
    command->function = config->loadaddr + payload_offset + 1;
    memset(&command->padding, 0x0, sizeof(command->padding));

    memmove(buffer + bootloader_offset, bootloader, bootloader_length);

    unsigned char *prepared_payload = construct_payload(config, bootloader_offset, bootloader_length);

    if (!prepared_payload) {
        printf("ERROR: failed to construct payload\n");
        free(buffer);
        return -1;
    }

    memmove(buffer + payload_offset, prepared_payload, sizeof(payload));

    free((void*)prepared_payload);

    *result = buffer;
    *result_length = command_length;

    return 0;
}

int dfu_boot(irecv_client_t client, const char *bootloader, size_t bootloader_length, bool debug) {
    if (validate_device(client) != 0) {
        printf("ERROR: device validation failed\n");
        return -1;
    }

    unsigned char *command;
    size_t command_length;

    if (construct_command(client, bootloader, bootloader_length, &command, &command_length) != 0) {
        printf("ERROR: failed to construct command\n");
        return -1;
    }

    if (debug) {
        int ret = save_command(client, command, command_length);
        irecv_close(client);
        return ret;
    }

    if (send_command(client, command, command_length) != 0) {
        printf("ERROR: failed to send command\n");
        return -1;
    }

    return 0;
}
