TOOLCHAIN_DIR ?= ../../xtensa-lx106-elf
TOOLS_PATH ?= $(TOOLCHAIN_DIR)/bin/xtensa-lx106-elf-
UZ_LIB ?= libuz.a
SDK_PATH ?= $(abspath ../)
CORE ?= $(abspath ../../../cores/esp8266)

BUILD_PATH = build
UZ_SRCS = $(BUILD_PATH)/uz.o

UZ_INCLUDE = -Iuzlib/src -I$(SDK_PATH)/include -I$(CORE)
BUILD_FLAGS = -c -Os -g -Wpointer-arith -Wl,-EL -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu++11 -ffunction-sections -fdata-sections
BUILD_DEFINES = -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__

ifdef WITH_DEBUG_PREFIX_MAP
EXTRA_FLAGS = -fdebug-prefix-map=$(PWD)= -fdebug-prefix-map=$(TOOLCHAIN_DIR)=xtensa-lx106-elf -gno-record-gcc-switches
endif

CXX=$(TOOLS_PATH)g++
AR=$(TOOLS_PATH)ar

$(BUILD_PATH)/%.h:
	@mkdir -p $(dir $@)
	@touch $@

$(BUILD_PATH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(BUILD_FLAGS) $(BUILD_DEFINES) $(UZ_INCLUDE) $(EXTRA_FLAGS) $< -o $@

$(UZ_LIB): $(UZ_SRCS)
	$(AR) cru $(UZ_LIB) $(UZ_SRCS)

all: $(UZ_LIB)

install: all
	cp -f $(UZ_LIB) $(SDK_PATH)/lib/$(UZ_LIB)

release: install

clean:
	@rm -rf $(BUILD_PATH) $(UZ_LIB)
