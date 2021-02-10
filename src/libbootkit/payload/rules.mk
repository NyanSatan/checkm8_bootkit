ARM_CC = $(TOOLCHAIN)/clang
ARM_CFLAGS = -mthumb
ARM_CFLAGS += -march=armv7-a
ARM_CFLAGS += -fno-builtin -nostdlib

ARM_OBJCOPY = $(TOOLCHAIN)/llvm-objcopy


PAYLOAD_DIR = $(LIBBOOTKIT_DIR)/payload

PAYLOAD_BIN = $(BUILD_PATH)/$(PAYLOAD_DIR)/payload.bin

PAYLOAD_SRC = $(PAYLOAD_DIR)/payload.S
PAYLOAD_OBJECT = $(addprefix $(BUILD_PATH)/, $(PAYLOAD_SRC:.S=.o))


$(PAYLOAD_BIN): $(PAYLOAD_OBJECT)
	@echo "\tbuilding payload"
	@$(DIR_HELPER)
	@$(ARM_OBJCOPY) -O binary $< $@

$(PAYLOAD_OBJECT): $(PAYLOAD_SRC)
	@$(DIR_HELPER)
	@$(ARM_CC) $(ARM_CFLAGS) -o $@ $<
