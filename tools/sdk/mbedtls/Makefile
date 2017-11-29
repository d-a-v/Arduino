LIB=libmbedtls.a
SDK=..
LIB_RELEASE=../lib/libmbedtls.a
INCLUDES_RELEASE=include
TOOLS=../../xtensa-lx106-elf/bin/xtensa-lx106-elf-
BUILD			= builder/build

CC			= $(TOOLS)gcc
BUILD_FLAGS 		+= -std=gnu99
AR			= $(TOOLS)ar
OC			= $(TOOLS)objcopy
OD			= $(TOOLS)objdump

OBJ = $(patsubst %.c,$(BUILD)/%.o,$(wildcard builder/library/*.c))
BUILD_FLAGS += -Wall -Wextra -c -Os -g -Wpointer-arith -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -ffunction-sections -fdata-sections
BUILD_DEFINES = -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ -DUSE_OPTIMIZE_PRINTF -DMBEDTLS_CONFIG_FILE='"config_esp.h"'
BUILD_INCLUDES = -I. -I$(BUILD) -Ibuilder/include -Ibuilder/include/mbedtls -I$(SDK)/include

all: builder/README.md $(LIB)

download: builder/README.md

builder/README.md:
	git submodule update --init builder

$(LIB_RELEASE): $(LIB)
	cp $(LIB) $(LIB_RELEASE)

.PHONY: $(LIB)
$(LIB):

install: $(LIB_RELEASE)
	rm -rf $(INCLUDES_RELEASE)
	mkdir -p $(INCLUDES_RELEASE)
	cp -a builder/include/mbedtls/* $(INCLUDES_RELEASE)
	cp config_esp.h $(INCLUDES_RELEASE)/config.h

clean:
	rm -rf builder/build $(LIB)

$(BUILD)/%.h:
	@mkdir -p $(dir $@)
	@touch $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(BUILD_FLAGS) $(BUILD_DEFINES) $(BUILD_INCLUDES) $< -o $@

$(LIB): $(OBJ)
	$(AR) cru $@ $(OBJ)

-include $(BUILD)/*.d
