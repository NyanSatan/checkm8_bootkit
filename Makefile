CC = clang
CFLAGS = -O3 -MMD

LD = clang

ARCHS = -arch x86_64 -arch arm64
MACOSX_MIN_VERSION = -mmacosx-version-min=10.7

STATIC_LIBS = static-libs

ifneq ($(wildcard $(STATIC_LIBS)),)
CFLAGS += $(ARCHS)
CFLAGS += $(MACOSX_MIN_VERSION)
LDFLAGS += $(ARCHS)
LDFLAGS += $(MACOSX_MIN_VERSION)
LDFLAGS += -L$(STATIC_LIBS) 
LDLIBS += -limobiledevice-glue-1.0
LDLIBS += -framework IOKit 
LDLIBS += -framework CoreFoundation
endif

LDLIBS += -lirecovery-1.0

VMACHO = vmacho

ARM_CC = xcrun -sdk iphoneos clang -arch armv7

BUILD_PATH = build

SOURCES = \
	src/main.c \
	src/libbootkit/libbootkit.c \
	src/libbootkit/libbootkit_watch.c

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

OBJECTS = $(addprefix $(BUILD_PATH)/, $(SOURCES:.c=.o))

RESULT = $(BUILD_PATH)/checkm8_bootkit

DIR_HELPER = mkdir -p $(@D)

all: $(PAYLOAD_H) $(PAYLOAD_WATCH_H) $(RESULT)
	@echo "%%%%% done building"

$(RESULT): $(OBJECTS)
	@echo "\tlinking"
	@$(DIR_HELPER)
	@$(LD) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(BUILD_PATH)/%.o: %.c
	@echo "\tbuilding C: $<"
	@$(DIR_HELPER)
	@$(CC) $(CFLAGS) -c $< -o $@

$(PAYLOAD_H): $(PAYLOAD_SRC)
	@echo "\tbuilding payload"
	@mkdir -p $(BUILD_PATH)
	@$(ARM_CC) -c $^ -o $(PAYLOAD_OBJ)
	@$(VMACHO) -f -C payload $(PAYLOAD_OBJ) $@

$(PAYLOAD_WATCH_H): $(PAYLOAD_WATCH_SRC)
	@echo "\tbuilding payload"
	@mkdir -p $(BUILD_PATH)
	@$(ARM_CC) -c $^ -o $(PAYLOAD_WATCH_OBJ)
	@$(VMACHO) -f -C payload_watch $(PAYLOAD_WATCH_OBJ) $@

.PHONY: clean clean-headers

clean:
	@rm -rf $(BUILD_PATH)
	@echo "%%%%% done cleaning"

clean-headers:
	@rm -rf $(PAYLOAD_H) $(PAYLOAD_WATCH_H)
	@echo "%%%%% done cleaning"

-include $(OBJECTS:.o=.d)
