#
# User defined properties
#

TOOLCHAIN ?= /opt/arm-arm64-llvm-11-toolchain/bin

#
# Do not touch these
#

CC = clang
CFLAGS = -Iinclude
CFLAGS += -MMD

LD = clang
LDFLAGS = -Llib
LDLIBS = -lirecovery-1.0
LDLIBS += -framework IOKit
LDLIBS += -framework CoreFoundation

BIN2C = bin2c


BUILD_PATH = build

SOURCES = \
	src/main.c

OBJECTS = $(addprefix $(BUILD_PATH)/, $(SOURCES:.c=.o))

RESULT = $(BUILD_PATH)/checkm8_bootkit

DIR_HELPER = mkdir -p $(@D)


.DEFAULT_GOAL := all


include src/libbootkit/rules.mk

all: libbootkit $(RESULT)
	@echo "%%%%% done building"

$(RESULT): $(OBJECTS)
	@echo "\tlinking"
	@$(DIR_HELPER)
	@$(LD) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(BUILD_PATH)/%.o: %.c
	@echo "\tbuilding C: $<"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	@rm -rf $(BUILD_PATH)/*
	@echo "%%%%% done cleaning"

-include $(OBJECTS:.o=.d) $(ENTRY_OBJECT:.o=.d)
