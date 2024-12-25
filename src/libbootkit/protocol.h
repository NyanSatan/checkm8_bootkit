#ifndef protocol_H
#define protocol_H

#include "config.h"

typedef struct {
    uint32_t magic;
    uint32_t magic2;
#define USB_COMMAND_EXEC_MAGIC 'exec'
    uint32_t function;
    uint8_t padding[4];
} usb_command_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t magic2;
#define USB_COMMAND_MEM_MAGIC 'memc'
    uint8_t padding[8];
    uint32_t dest_ptr;
    uint32_t src_ptr;
    uint32_t length;
} usb_command_memc_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t magic2;
#define USB_COMMAND_DONE_MAGIC 'done'
    uint32_t ret;
    uint8_t padding[4];
} usb_command_done_t;

int read32(irecv_client_t client, const rom_config_t *config, uint32_t address, uint32_t *dest);
int write32(irecv_client_t client, const rom_config_t *config, uint32_t address, uint32_t value);

#define MAX_ARGS 8
#define AUX_DATA_START  (sizeof(usb_command_t) + sizeof(uint32_t) * MAX_ARGS)
int execute(irecv_client_t client, const rom_config_t *config, uint8_t *output, size_t output_len, uint32_t address, uint32_t args[MAX_ARGS], uint8_t *aux_data, size_t aux_data_len);

int validate_device(irecv_client_t client);

int send_command(irecv_client_t client, unsigned char *command, size_t length);
int trigger_command(irecv_client_t client, unsigned char *response, size_t response_length);

int save_command(irecv_client_t client, unsigned char *command, size_t length);

#endif
