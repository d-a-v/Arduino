
# uzlib submodule for esp8266/arduino

SDK			=..
TOOLS			=../../xtensa-lx106-elf/bin/xtensa-lx106-elf-

CC			= $(TOOLS)gcc
BUILD_FLAGS 		+= -std=gnu99

AR			= $(TOOLS)ar
OC			= $(TOOLS)objcopy
OD			= $(TOOLS)objdump

BUILD			?= build
BUILD_FLAGS += -Wall -Wextra -c -Os -g -Wpointer-arith -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -ffunction-sections -fdata-sections
BUILD_DEFINES = -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ -DUSE_OPTIMIZE_PRINTF -DTARGET=$(target)

## tinflib  -  tiny inflate library (inflate, gzip, zlib)
## GCC makefile (Linux, FreeBSD, BeOS and QNX)
## Copyright (c) 2003 by Joergen Ibsen / Jibz
## All Rights Reserved
## http://www.ibsensoftware.com/

lib  = ../lib/libuz.a
target  = builder/build/libuz.a
objectlist = tinflate.o tinfgzip.o tinfzlib.o adler32.o crc32.o defl_static.o genlz77.o
objects = $(objectlist:%=builder/build/%)

all: builder/build rebuild

install: all $(lib)
	mkdir -p include
	cp -a builder/src/*.h include/

$(lib): $(target)
	cp $(target) $(lib)

rebuild:
	$(MAKE) build

build: $(target)

builder/build:
	git submodule update --init --recursive builder
	mkdir builder/build

$(target): $(objects)
	$(AR) cru $@ $^

builder/build/%.o : builder/src/%.c
	$(CC) $(BUILD_FLAGS) $(BUILD_DEFINES) -o $@ -c $<

clean:
	rm -rf builder/build
