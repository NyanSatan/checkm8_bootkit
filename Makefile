MAC_CC = clang
IOS_CC = xcrun --sdk iphoneos clang

IOS_CODESIGN = codesign
IOS_ENT = ent.plist

MAC_ARCH = -arch x86_64 -arch arm64
IOS_ARCH = -arch armv7 -arch arm64

IOS_CFLAGS = -miphoneos-version-min=6.0
IOS_CFLAGS += -Iinclude
IOS_IOKIT_LINK = ln -fsh $(shell xcrun --sdk macosx --show-sdk-path)/System/Library/Frameworks/IOKit.framework/Versions/Current/Headers ./include/IOKit

MAC_CFLAGS = -mmacosx-version-min=10.8

CFLAGS = -O3
CFLAGS += -Ililirecovery

LDFLAGS = -framework IOKit -framework CoreFoundation

ARM_CC = xcrun -sdk iphoneos clang -arch armv7
VMACHO = vmacho

BUILD_PATH = build

SOURCES = \
	src/main.c \
	lilirecovery/lilirecovery.c \
	src/libbootkit/boot.c \
	src/libbootkit/ops.c \
	src/libbootkit/dfu.c \
	src/libbootkit/protocol.c

PAYLOAD_H = \
	src/libbootkit/payload.h

PAYLOAD_OBJ = \
	$(BUILD_PATH)/payload.o

PAYLOAD_SRC = \
	src/libbootkit/payloads/payload.S

PAYLOAD_WATCH_H = \
	src/libbootkit/payload_watch.h

PAYLOAD_WATCH_OBJ = \
	$(BUILD_PATH)/payload_watch.o

PAYLOAD_WATCH_SRC = \
	src/libbootkit/payloads/payload_watch.S

MAC_RESULT = $(BUILD_PATH)/checkm8_bootkit
IOS_RESULT = $(BUILD_PATH)/checkm8_bootkit_ios

DIR_HELPER = mkdir -p $(@D)

.PHONY: all mac ios clean clean-headers

all: $(PAYLOAD_H) $(PAYLOAD_WATCH_H) $(MAC_RESULT) $(IOS_RESULT)
	@echo "%%%%% done building"

mac: $(MAC_RESULT)

ios: $(IOS_RESULT)

$(MAC_RESULT): $(SOURCES)
	@echo "\tbuilding checkm8_bootkit for Mac"
	@$(DIR_HELPER)
	@$(MAC_CC) $(MAC_ARCH) $(MAC_CFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(IOS_RESULT): $(SOURCES)
	@echo "\tbuilding checkm8_bootkit for iOS"
	@$(DIR_HELPER)
	@$(IOS_IOKIT_LINK)
	@$(IOS_CC) $(IOS_ARCH) $(IOS_CFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@$(IOS_CODESIGN) -s - -f --entitlements $(IOS_ENT) $@

$(BUILD_PATH)/%.o: %.c
	@echo "\tbuilding C: $<"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) -c $< -o $@

$(PAYLOAD_H): $(PAYLOAD_SRC)
	@echo "\tbuilding boot payload"
	@$(DIR_HELPER)
	@$(ARM_CC) -c $^ -o $(PAYLOAD_OBJ)
	@$(VMACHO) -f -C payload $(PAYLOAD_OBJ) $@

$(PAYLOAD_WATCH_H): $(PAYLOAD_WATCH_SRC)
	@echo "\tbuilding watch boot payload"
	@$(DIR_HELPER)
	@$(ARM_CC) -c $^ -o $(PAYLOAD_WATCH_OBJ)
	@$(VMACHO) -f -C payload_watch $(PAYLOAD_WATCH_OBJ) $@

clean:
	@rm -rf $(BUILD_PATH)
	@echo "%%%%% done cleaning"

clean-headers:
	@rm -rf $(PAYLOAD_H) $(PAYLOAD_WATCH_H)
	@echo "%%%%% done cleaning headers"
