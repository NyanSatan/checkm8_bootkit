#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <lilirecovery.h>

#include "dfu.h"
#include "protocol.h"
#include "config.h"
#include "payload.h"
#include "payload_watch.h"

#define ARM_RESET_VECTOR 0xEA00000E

/*
 * Watch-specific stuff
 */

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
} payload_offsets_watch_t;

static unsigned char *construct_payload_watch(const rom_config_t *config) {
    printf("constructing payload...\n");
    
    unsigned char *payload_copy = malloc(sizeof(payload_watch));
    if (!payload_copy) {
        printf("out of memory\n");
        return NULL;
    }

    memmove(payload_copy, &payload_watch, sizeof(payload_watch));

    static const uint32_t magic = 0xDEAD0001;
    
    unsigned char *offset = memmem(payload_copy, sizeof(payload_watch), &magic, sizeof(magic));
    if (!offset) {
        goto improper_payload;
    }

    if (sizeof(payload_watch) - (size_t)(offset - payload_copy) < sizeof(payload_offsets_watch_t)) {
        goto improper_payload;
    }

    payload_offsets_watch_t *payload_offsets = (payload_offsets_watch_t *)offset;

    payload_offsets->loadaddr = config->loadaddr;
    payload_offsets->security_consolidate_environment = config->boot_config_watch.security_consolidate_environment;
    payload_offsets->security_sidp_seal_rom_manifest = config->boot_config_watch.security_sidp_seal_rom_manifest;
    payload_offsets->platform_get_boot_trampoline = config->boot_config_watch.platform_get_boot_trampoline;
    payload_offsets->platform_bootprep = config->boot_config_watch.platform_bootprep;
    payload_offsets->usb_controller_stop = config->boot_config_watch.usb_controller_stop;
    payload_offsets->interrupt_mask_all = config->boot_config_watch.interrupt_mask_all;
    payload_offsets->timer_stop_all = config->boot_config_watch.timer_stop_all;
    payload_offsets->clocks_quiesce = config->boot_config_watch.clocks_quiesce;
    payload_offsets->enter_critical_section = config->boot_config_watch.enter_critical_section;
    payload_offsets->arch_cpu_quiesce = config->boot_config_watch.arch_cpu_quiesce;

    goto success;

improper_payload:
    free(payload_copy);
    printf("improper payload\n");
    return NULL;

success:
    return payload_copy;
}

static int construct_command_watch(irecv_client_t client,
                      const rom_config_t *config,
                      const uint8_t *bootloader,
                      size_t bootloader_length,
                      unsigned char **result,
                      size_t *result_length) {

    printf("constructing command...\n");

    if (*(uint32_t*)bootloader != ARM_RESET_VECTOR) {
        printf("provided bootloader doesn't seem to be an ARM image\n");
        return -1;
    }

    size_t command_length = sizeof(payload_watch) + bootloader_length;

    if (command_length > config->loadsize) {
        printf("resulting command is too big, use smaller bootloader (at least %lu smaller)\n", command_length - config->loadsize);
        return -1;
    }

    unsigned char *buffer = malloc(command_length);
    if (!buffer) {
        printf("out of memory");
        return -1;
    }

    memset(buffer, 0x0, command_length);
    memmove(buffer, bootloader, bootloader_length);

    unsigned char *prepared_payload = construct_payload_watch(config);
    if (!prepared_payload) {
        printf("failed to construct payload\n");
        free(buffer);
        return -1;
    }

    memmove(buffer + bootloader_length, prepared_payload, sizeof(payload_watch));

    free(prepared_payload);

    *result = buffer;
    *result_length = command_length;

    return 0;
}


int dfu_boot_watch(irecv_client_t client, const uint8_t *bootloader, size_t bootloader_length, bool debug) {
    if (validate_device(client) != 0) {
        printf("device validation failed\n");
        return -1;
    }

    const struct irecv_device_info *info = irecv_get_device_info(client);
    const rom_config_t *config = get_config(info->cpid);
    if (!config) {
        printf("no config available for CPID:%04X\n", info->cpid);
        return -1;
    }

    unsigned char *command = NULL;
    size_t command_length = 0;

    if (construct_command_watch(client, config, bootloader, bootloader_length, &command, &command_length) != 0) {
        printf("failed to construct command\n");
        return -1;
    }

    int ret = -1;

    if (debug) {
        ret = save_command(client, command, command_length);
        goto out;
    }

    if (write32(client, config, config->boot_config_watch.func_ptr, config->loadaddr + bootloader_length + 1) != 0) {
        printf("failed to overwrite function pointer\n");
        goto out;
    }

    if (send_command(client, command, command_length) != 0) {
        printf("failed to send command\n");
        goto out;
    }

    printf("requesting DFU abort\n");

    irecv_usb_control_transfer(client, 0x21, 4, 0, 0, NULL, 0, USB_SMALL_TIMEOUT);

    ret = 0;

out:
    if (command) {
        free(command);
    }

    return ret;
}

/*
 * Older platform stuff
 */

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
    uint32_t prepare_and_jump;
} payload_offsets_t;

static unsigned char *construct_payload(const rom_config_t *config, off_t bootloader_offset, size_t bootloader_length) {
    printf("constructing payload...\n");
    
    unsigned char *payload_copy = malloc(sizeof(payload));
    if (!payload_copy) {
        printf("out of memory\n");
        return NULL;
    }

    memmove(payload_copy, &payload, sizeof(payload));

    static const uint32_t magic = 0xDEAD0001;
    
    unsigned char *offset = memmem(payload_copy, sizeof(payload), &magic, sizeof(magic));
    if (!offset) {
        printf("improper payload\n");
        return NULL;
    }

    if (sizeof(payload) - (size_t)(offset - payload_copy) < sizeof(payload_offsets_t)) {
        printf("improper payload\n");
        return NULL;
    }

    payload_offsets_t *payload_offsets = (payload_offsets_t *)offset;

    payload_offsets->loadaddr = config->loadaddr;
    payload_offsets->imageaddr = config->loadaddr + bootloader_offset;
    payload_offsets->imagesize = bootloader_length;
    payload_offsets->memmove = config->boot_config.memmove;
    payload_offsets->platform_get_boot_trampoline = config->boot_config.platform_get_boot_trampoline;
    payload_offsets->platform_bootprep = config->boot_config.platform_bootprep;
    payload_offsets->usb_quiesce_no_free = config->boot_config.usb_quiesce_no_free;
    payload_offsets->interrupt_mask_all = config->boot_config.interrupt_mask_all;
    payload_offsets->timer_stop_all = config->boot_config.timer_stop_all;
    payload_offsets->clocks_quiesce = config->boot_config.clocks_quiesce;
    payload_offsets->enter_critical_section = config->boot_config.enter_critical_section;
    payload_offsets->arch_cpu_quiesce = config->boot_config.arch_cpu_quiesce;
    payload_offsets->prepare_and_jump = config->boot_config.prepare_and_jump;

    return payload_copy;
}

static int construct_command(irecv_client_t client,
                      const uint8_t *bootloader,
                      size_t bootloader_length,
                      unsigned char **result,
                      size_t *result_length) {

    printf("constructing command...\n");

    if (*(uint32_t *)bootloader != ARM_RESET_VECTOR) {
        printf("provided bootloader doesn't seem to be an ARM image\n");
        return -1;
    }

    const struct irecv_device_info *info = irecv_get_device_info(client);

    const rom_config_t *config = get_config(info->cpid);
    if (!config) {
        printf("no config available for CPID:%04X\n", info->cpid);
        return -1;
    }

    size_t command_length = sizeof(payload) + bootloader_length + sizeof(usb_command_t);

    if (command_length > config->loadsize) {
        printf("resulting command is too big, use smaller bootloader (at least %lu smaller)\n", command_length - config->loadsize);
        return -1;
    }

    unsigned char *buffer = malloc(command_length);
    if (!buffer) {
        printf("out of memory");
        return -1;
    }

    memset(buffer, 0x0, command_length);

    off_t bootloader_offset = sizeof(usb_command_t);
    off_t payload_offset = bootloader_offset + bootloader_length;

    usb_command_t *command = (usb_command_t *)buffer;
    command->magic = USB_COMMAND_EXEC_MAGIC;
    command->magic2 = USB_COMMAND_EXEC_MAGIC;
    command->function = config->loadaddr + payload_offset + 1;
    memset(&command->padding, 0x0, sizeof(command->padding));

    memmove(buffer + bootloader_offset, bootloader, bootloader_length);

    unsigned char *prepared_payload = construct_payload(config, bootloader_offset, bootloader_length);
    if (!prepared_payload) {
        printf("failed to construct payload\n");
        free(buffer);
        return -1;
    }

    memmove(buffer + payload_offset, prepared_payload, sizeof(payload));

    free(prepared_payload);

    *result = buffer;
    *result_length = command_length;

    return 0;
}

int dfu_boot(irecv_client_t client, const uint8_t *bootloader, size_t bootloader_length, bool debug) {
    if (validate_device(client) != 0) {
        printf("device validation failed\n");
        return -1;
    }

    unsigned char *command = NULL;
    size_t command_length = 0;

    if (construct_command(client, bootloader, bootloader_length, &command, &command_length) != 0) {
        printf("failed to construct command\n");
        return -1;
    }

    int ret = -1;

    if (debug) {
        ret = save_command(client, command, command_length);
        goto out;
    }

    if (send_command(client, command, command_length) != 0) {
        printf("failed to send command\n");
        goto out;
    }

    trigger_command(client, NULL, 0);

    ret = 0;

out:
    if (command) {
        free(command);
    }

    return ret;
}
