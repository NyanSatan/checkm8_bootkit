#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <payload.h>

#include "libbootkit.h"
#include "config.h"


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

const config_t *get_config(uint32_t cpid) {

    for (int i = 0; i < sizeof(configs) / sizeof(config_t); i++) {
        const config_t *config = &configs[i];

        if (config->cpid == cpid)
            return config;
    }

    return NULL;
}

#define MAX_PACKET_SIZE     0x800
#define USB_SMALL_TIMEOUT   100
#define USB_TIMEOUT         5000


size_t min(size_t first, size_t second) {
    if (first < second)
        return first;
    else
        return second;
}

int send_data(irecv_client_t client, unsigned char *command, size_t length) {
    size_t index = 0;

    while (index < length) {
        size_t amount = min(length - index, MAX_PACKET_SIZE);
        if (irecv_usb_control_transfer(client, 0x21, 1, 0, 0, command + index, amount, USB_TIMEOUT) != amount)
            return -1;
        index += amount;
    }

    return 0;
}

int request_image_validation(irecv_client_t client, bool needs_reset) {
    unsigned char dummy_data[6];
    memset(&dummy_data, 0x0, sizeof(dummy_data));

    if (send_data(client, (unsigned char*)&dummy_data, sizeof(dummy_data)) != 0) {
        printf("ERROR: failed to send dummy data\n");
        return -1;        
    }

    irecv_usb_control_transfer(client, 0x21, 1, 0, 0, NULL, 0, USB_SMALL_TIMEOUT);
    irecv_usb_control_transfer(client, 0xA1, 3, 0, 0, (unsigned char*)&dummy_data, sizeof(dummy_data), USB_SMALL_TIMEOUT);
    irecv_usb_control_transfer(client, 0xA1, 3, 0, 0, (unsigned char*)&dummy_data, sizeof(dummy_data), USB_SMALL_TIMEOUT);

    if (needs_reset) {
        irecv_reset(client);
    }

    return 0;
}

int send_command(irecv_client_t client, unsigned char *command, size_t length) {
    printf("sending command...\n");

    if (request_image_validation(client, false) != 0) {
        printf("ERROR: failed to prepare command sending\n");
        return -1;
    }

    if (send_data(client, command, length) != 0) {
        printf("ERROR: failed to send command buffer\n");
        return -1;        
    }

    return 0;
}

int trigger_command(irecv_client_t client, unsigned char *response, size_t response_length) {
    if (!response_length) {
        uint8_t dummy_data;
        return irecv_usb_control_transfer(client, 0xA1, 2, 0xFFFF, 0, (unsigned char*)&dummy_data, sizeof(dummy_data), USB_TIMEOUT);
    } else {
        return irecv_usb_control_transfer(client, 0xA1, 2, 0xFFFF, 0, (unsigned char*)response, response_length, USB_TIMEOUT);
    }
}

int read_mem32(irecv_client_t client, const config_t *config, uint32_t address, uint32_t *dest) {
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

int write_mem32(irecv_client_t client, const config_t *config, uint32_t address, uint32_t value) {
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

unsigned char *construct_payload(const config_t *config) {
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
        goto improper_payload;
    }

    if (sizeof(payload) - (size_t)(offset - payload_copy) < sizeof(payload_offsets_t)) {
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

#define ARM_RESET_VECTOR 0xEA00000E

int construct_command(irecv_client_t client,
                      const config_t *config,
                      const char *bootloader,
                      size_t bootloader_length,
                      unsigned char **result,
                      size_t *result_length) {

    printf("constructing command...\n");

    if (*(uint32_t*)bootloader != ARM_RESET_VECTOR) {
        printf("ERROR: provided bootloader doesn't seem to be an ARM image\n");
        return -1;
    }

    size_t command_length = sizeof(payload) + bootloader_length;

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

    memmove(buffer + bootloader_length, prepared_payload, sizeof(payload));

    free((void*)prepared_payload);

    *result = buffer;
    *result_length = command_length;

    return 0;
}

int validate_device(irecv_client_t client) {
    const struct irecv_device_info *info = irecv_get_device_info(client);

    int mode;

    if (irecv_get_mode(client, &mode) != IRECV_E_SUCCESS) {
        printf("ERROR: failed to get device mode\n");
        return -1;
    }

    if (mode != IRECV_K_DFU_MODE) {
        printf("ERROR: non-DFU device found\n");
        return -1;
    }

    if (!info->srtg) {
        printf("ERROR: soft-DFU device found\n");
        return -1;
    }

    if (!strstr(info->serial_string, "PWND:[checkm8]")) {
        printf("ERROR: non-pwned-DFU device found\n");
        return -1;
    }

    return 0;
}

int save_command(irecv_client_t client, unsigned char *command, size_t length) {
    const struct irecv_device_info *info = irecv_get_device_info(client);

    char path[40];
    snprintf((char *)&path, sizeof(path), "/tmp/%04X-%016llX_%08X", info->cpid, info->ecid, arc4random_uniform(UINT32_MAX));

    int fd = open(path, O_WRONLY | O_CREAT, 0644);

    if (fd < 0) {
        printf("ERROR: failed to create output file\n");
        return -1;
    }

    if (write(fd, command, length) != length) {
        printf("ERROR: failed to write to output file\n");
        return -1;
    }

    printf("written to %s\n", path);

    return 0;
}

int dfu_boot(irecv_client_t client, const char *bootloader, size_t bootloader_length, bool debug) {
    if (validate_device(client) != 0) {
        printf("ERROR: device validation failed\n");
        return -1;
    }

    const struct irecv_device_info *info = irecv_get_device_info(client);
    const config_t *config = get_config(info->cpid);
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
