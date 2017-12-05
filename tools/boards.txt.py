#!/usr/bin/env python

# boards.txt python builder for esp8266/Arduino
# Copyright (C) 2017 community
# Permission is hereby granted, free of charge, to any person who buy it,
# use it, break it, fix it, trash it, change it, mail - upgrade it, charge
# it, point it, zoom it, press it, snap it, work it, quick - erase it, write
# it, cut it, paste it, save it, load it, check it, quick - rewrite it, plug
# it, play it, burn it, rip it, drag and drop it, zip - unzip it, lock it,
# fill it, call it, find it, view it, code it, jam - unlock it, surf it,
# scroll it, pause it, click it, cross it, crack it, switch - update it,
# name it, rate it, tune it, print it, scan it, send it, fax - rename it,
# touch it, bring it, pay it, watch it, turn it, leave it, start - format
# it.

# board descriptor:
#    name     display name
#    opts:    specific entries dicts (overrides same entry in macros)
#    macro:   common entries
#        unmodifiable parameters:
#            resetmethod_ck/_nodemcu:        fixed reset method
#            flashmode_qio/_dio/_qout/_dout: fixed flash mode
#            flashfreq_40/_80:               fixed flash frequency
#        selection menu:
#            resetmethod_menu            menus for reset method
#            crystalfreq/flashfreq_menu: menus for crystal/flash frequency selection
#            flashmode_menu:             menus for flashmode selection (dio/dout/qio/qout)
#            512K/1M/2M/4M/8M/16M:       menus for flash & SPIFFS size
#            lwip/lwip2                  menus for available lwip versions

import os
import sys
import collections
import getopt

# serial upload speed order in menu
# default is 115 for every board unless specified with 'serial' in board
# or by user command line

speeds = collections.OrderedDict([
    (   '9', [ 's9', 's57', 's115', 's230', 's256', 's460', 's512', 's921' ]),
    (  '57', [ 's57', 's9', 's115', 's230', 's256', 's460', 's512', 's921' ]),
    ( '115', [ 's115', 's9', 's57', 's230', 's256', 's460', 's512', 's921' ]),
    ( '230', [ 's230', 's9', 's57', 's115', 's256', 's460', 's512', 's921' ]),
    ( '256', [ 's256', 's9', 's57', 's115', 's230', 's460', 's512', 's921' ]),
    ( '460', [ 's460', 's9', 's57', 's115', 's230', 's256', 's512', 's921' ]),
    ( '512', [ 's512', 's9', 's57', 's115', 's230', 's256', 's460', 's921' ]),
    ( '921', [ 's921', 's9', 's57', 's115', 's230', 's256', 's460', 's512' ]),
    ])

# boards list

boards = collections.OrderedDict([
    ( 'generic', {
        'name': 'Generic ESP8266 Module',
        'opts': {
            '.build.board': 'ESP8266_GENERIC',
            },
        'macro': [
            'resetmethod_menu',
            'crystalfreq_menu',
            'flashfreq_menu',
            'flashmode_menu',
            '512K', '1M', '2M', '4M', '8M', '16M',
            'led',
            ],
    }),
    ( 'esp8285', {
        'name': 'Generic ESP8285 Module',
        'opts': {
            '.build.board': 'ESP8266_ESP01',
            },
        'macro': [
            'resetmethod_menu',
            'crystalfreq_menu',
            'flashmode_dout',
            'flashfreq_40',
            '1M',
            'led',
            ],
    }),
    ( 'espduino', {
        'name': 'ESPDuino (ESP-13 Module)',
        'opts': {
            '.build.board': 'ESP8266_ESP13',
            '.build.variant': 'ESPDuino',
            '.menu.ResetMethod.ck': 'ESPduino-V1',
            '.menu.ResetMethod.ck.upload.resetmethod': 'nodemcu',
            '.menu.ResetMethod.ESPduino': 'ESPduino-V2',
            '.menu.ResetMethod.ESPduino.upload.resetmethod': 'nodemcu',
            '.menu.UploadTool.esptool': 'Serial',
            '.menu.UploadTool.esptool.upload.tool': 'esptool',
            '.menu.UploadTool.esptool.upload.verbose': '-vv',
            '.menu.UploadTool.espota': 'OTA',
            '.menu.UploadTool.espota.upload.tool': 'espota',
            },
        'macro': [
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            ],
    }),
    ( 'huzzah', {
        'name': 'Adafruit HUZZAH ESP8266',
        'opts': {
            '.build.board': 'ESP8266_ESP12',
            '.build.variant': 'adafruit',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_qio',
            'flashfreq_40',
            '4M',
            ],
    }),
    ( 'espresso_lite_v1', {
        'name': 'ESPresso Lite 1.0',
        'opts': {
            '.build.board': 'ESP8266_ESPRESSO_LITE_V1',
            '.build.variant': 'espresso_lite_v1',
            },
        'macro': [
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            'resetmethod_menu',
            ],
    }),
    ( 'espresso_lite_v2', {
        'name': 'ESPresso Lite 2.0',
        'opts': {
            '.build.board': 'ESP8266_ESPRESSO_LITE_V2',
            '.build.variant': 'espresso_lite_v2',
            },
        'macro': [
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            'resetmethod_menu',
            ],
    }),
    ( 'phoenix_v1', {
        'name': 'Phoenix 1.0',
        'opts': {
            '.build.board': 'ESP8266_PHOENIX_V1',
            '.build.variant': 'phoenix_v1',
            },
        'macro': [
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            'resetmethod_menu',
            ],
    }),
    ( 'phoenix_v2', {
        'name': 'Phoenix 2.0',
        'opts': {
            '.build.board': 'ESP8266_PHOENIX_V2',
            '.build.variant': 'phoenix_v2',
            },
        'macro': [
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            'resetmethod_menu',
            ],
    }),
    ( 'nodemcu', {
        'name': 'NodeMCU 0.9 (ESP-12 Module)',
        'opts': {
            '.build.board': 'ESP8266_NODEMCU',
            '.build.variant': 'nodemcu',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_qio',
            'flashfreq_40',
            '4M',
            ],
    }),
    ( 'nodemcuv2', {
        'name': 'NodeMCU 1.0 (ESP-12E Module)',
        'opts': {
            '.build.board': 'ESP8266_NODEMCU',
            '.build.variant': 'nodemcu',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            ],
    }),
    ( 'modwifi', {
        'name': 'Olimex MOD-WIFI-ESP8266(-DEV)',
        'opts': {
            '.build.board': 'MOD_WIFI_ESP8266',
            },
        'macro': [
            'resetmethod_ck',
            'flashmode_qio',
            'flashfreq_40',
            '2M',
            ],
    }),
    ( 'thing', {
        'name': 'SparkFun ESP8266 Thing',
        'opts': {
            '.build.board': 'ESP8266_THING',
            '.build.variant': 'thing',
            },
        'macro': [
            'resetmethod_ck',
            'flashmode_qio',
            'flashfreq_40',
            '512K',
            ],
    }),
    ( 'thingdev', {
        'name': 'SparkFun ESP8266 Thing Dev',
        'opts': {
            '.build.board': 'ESP8266_THING_DEV',
            '.build.variant': 'thing',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_dio',
            'flashfreq_40',
            '512K',
            ],
    }),
    ( 'esp210', {
        'name': 'SweetPea ESP-210',
        'opts': {
            '.build.board': 'ESP8266_ESP210',
            },
        'macro': [
            'resetmethod_ck',
            'flashmode_qio',
            'flashfreq_40',
            '4M',
            ],
        'serial': '57',
    }),
    ( 'd1_mini', {
        'name': 'WeMos D1 R2 & mini',
        'opts': {
            '.build.board': 'ESP8266_WEMOS_D1MINI',
            '.build.variant': 'd1_mini',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            ],
        'serial': '921',
    }),
    ( 'd1_mini_pro', {
        'name': 'WeMos D1 mini Pro',
        'opts': {
            '.build.board': 'ESP8266_WEMOS_D1MINIPRO',
            '.build.variant': 'd1_mini',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_dio',
            'flashfreq_40',
            '16M',
            ],
        'serial': '921',
    }),
    ( 'd1_mini_lite', {
        'name': 'Wemos D1 mini Lite',
        'opts': {
            '.build.board': 'ESP8266_WEMOS_D1MINILITE',
            '.build.variant': 'd1_mini',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_dout',
            'flashfreq_40',
            '1M',
            ],
        'serial': '921',
    }),
    ( 'd1', {
        'name': 'WeMos D1 R1',
        'opts': {
            '.build.board': 'ESP8266_WEMOS_D1MINI',
            '.build.variant': 'd1',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            ],
        'serial': '921',
    }),
    ( 'espino', {
        'name': 'ESPino (ESP-12 Module)',
        'opts': {
            '.build.board': 'ESP8266_ESP12',
            '.build.variant': 'espino',
            },
        'macro': [
            'resetmethod_menu',
            'flashmode_qio',
            'flashfreq_40',
            '4M',
            ]
    }),
    ( 'espinotee', {
        'name': 'ThaiEasyElec\'s ESPino',
        'opts': {
            '.build.board': 'ESP8266_ESP13',
            '.build.variant': 'espinotee',
            },
        'macro': [
            'resetmethod_nodemcu',
            'flashmode_qio',
            'flashfreq_40',
            '4M',
            ],
    }),
    ( 'wifinfo', {
        'name': 'WifInfo',
        'opts': {
            '.build.board': 'WIFINFO',
            '.build.variant': 'wifinfo',
            '.menu.ESPModule.ESP07192': 'ESP07 (1M/192K SPIFFS)',
            '.menu.ESPModule.ESP07192.build.board': 'ESP8266_ESP07',
            '.menu.ESPModule.ESP07192.build.flash_size': '1M',
            '.menu.ESPModule.ESP07192.build.flash_ld': 'eagle.flash.1m192.ld',
            '.menu.ESPModule.ESP07192.build.spiffs_start': '0xCB000',
            '.menu.ESPModule.ESP07192.build.spiffs_end': '0xFB000',
            '.menu.ESPModule.ESP07192.build.spiffs_blocksize': '4096',
            '.menu.ESPModule.ESP07192.upload.maximum_size': '827376',
            '.menu.ESPModule.ESP12': 'ESP12 (4M/1M SPIFFS)',
            '.menu.ESPModule.ESP12.build.board': 'ESP8266_ESP12',
            '.menu.ESPModule.ESP12.build.flash_size': '4M',
            '.menu.ESPModule.ESP12.build.flash_ld': 'eagle.flash.4m1m.ld',
            '.menu.ESPModule.ESP12.build.spiffs_start': '0x300000',
            '.menu.ESPModule.ESP12.build.spiffs_end': '0x3FB000',
            '.menu.ESPModule.ESP12.build.spiffs_blocksize': '8192',
            '.menu.ESPModule.ESP12.build.spiffs_pagesize': '256',
            '.menu.ESPModule.ESP12.upload.maximum_size': '1044464',
            },
        'macro': [
            'flashmode_qio',
            'flashfreq_menu',
            '1M',
            ]
    }),
    ( 'arduino-esp8266', {
        'name': 'Arduino',
        'opts': {
            '.build.board': 'ESP8266_ARDUINO',
            '.menu.BoardModel.primo': 'Primo',
            '.menu.BoardModel.primo.build.board': 'ESP8266_ARDUINO_PRIMO',
            '.menu.BoardModel.primo.build.variant': 'arduino_spi',
            '.menu.BoardModel.primo.build.extra_flags': '-DF_CRYSTAL=40000000',
            '.menu.BoardModel.unowifideved': 'Uno WiFi',
            '.menu.BoardModel.unowifideved.build.board': 'ESP8266_ARDUINO_UNOWIFI',
            '.menu.BoardModel.unowifideved.build.variant': 'arduino_uart',
            '.menu.BoardModel.unowifideved.build.extra_flags=-DF_CRYSTAL': '40000000',
            '.menu.BoardModel.starottodeved': 'Star OTTO',
            '.menu.BoardModel.starottodeved.build.variant': 'arduino_uart',
            '.menu.BoardModel.starottodeved.build.board': 'ESP8266_ARDUINO_STAR_OTTO',
            '.menu.BoardModel.starottodeved.build.extra_flags': '-DF_CRYSTAL=40000000',
            },
        'macro': [
            'flashmode_qio',
            'flashfreq_40',
            '4M',
            ],
    }),
    ( 'gen4iod', {
        'name': '4D Systems gen4 IoD Range',
        'opts': {
            '.build.board': 'GEN4_IOD',
            '.build.f_cpu': '160000000L',
            '.build.variant': 'generic',
            },
        'macro': [
            'flashmode_qio',
            'flashfreq_80',
            '512K',
            ],
    }),
    ( 'oak', {
        'name': 'DigiStump Oak',
        'opts': {
            '.build.board': 'ESP8266_OAK',
            '.build.variant': 'oak',
            '.upload.maximum_size': '1040368',
            },
        'macro': [
            'flashmode_dio',
            'flashfreq_40',
            '4M',
            ],
        'serial': '921',
    }),
    ])

################################################################

macros = {
    'defaults': collections.OrderedDict([
        ( '.upload.tool', 'esptool' ),
        ( '.upload.maximum_data_size', '81920' ),
        ( '.upload.wait_for_upload_port', 'true' ),
        ( '.serial.disableDTR', 'true' ),
        ( '.serial.disableRTS', 'true' ),
        ( '.build.mcu', 'esp8266' ),
        ( '.build.core', 'esp8266' ),
        ( '.build.variant', 'generic' ),
        ( '.build.spiffs_pagesize', '256' ),
        ( '.build.debug_port', '' ),
        ( '.build.debug_level', '' ),
        ]),

    #######################

    'cpufreq_menu': collections.OrderedDict([
        ( '.menu.CpuFrequency.80', '80 MHz' ),
        ( '.menu.CpuFrequency.80.build.f_cpu', '80000000L' ),
        ( '.menu.CpuFrequency.160', '160 MHz' ),
        ( '.menu.CpuFrequency.160.build.f_cpu', '160000000L' ),
        ]),
    
    'crystalfreq_menu': collections.OrderedDict([
        ( '.menu.CrystalFreq.26', '26 MHz' ),
        ( '.menu.CrystalFreq.40', '40 MHz' ),
        ( '.menu.CrystalFreq.40.build.extra_flags', '-DF_CRYSTAL=40000000' ),
        ]),
        
    'flashfreq_menu': collections.OrderedDict([
        ( '.menu.FlashFreq.40', '40MHz' ),
        ( '.menu.FlashFreq.40.build.flash_freq', '40' ),
        ( '.menu.FlashFreq.80', '80MHz' ),
        ( '.menu.FlashFreq.80.build.flash_freq', '80' ),
        ]),
        
    'flashfreq_40': collections.OrderedDict([
        ( '.build.flash_freq', '40' ),
        ]),

    'flashfreq_80': collections.OrderedDict([
        ( '.build.flash_freq', '80' ),
        ]),

    ####################### menu.resetmethod
    
    'resetmethod_menu': collections.OrderedDict([
        ( '.menu.ResetMethod.ck', 'ck' ),
        ( '.menu.ResetMethod.ck.upload.resetmethod', 'ck' ),
        ( '.menu.ResetMethod.nodemcu', 'nodemcu' ),
        ( '.menu.ResetMethod.nodemcu.upload.resetmethod', 'nodemcu' ),
        ]),
    
    ####################### upload.resetmethod
    
    'resetmethod_ck': collections.OrderedDict([
        ( '.upload.resetmethod', 'ck' ),
        ]),
    
    'resetmethod_nodemcu': collections.OrderedDict([
        ( '.upload.resetmethod', 'nodemcu' ),
        ]),

    ####################### menu.FlashMode
    
    'flashmode_menu': collections.OrderedDict([
        ( '.menu.FlashMode.qio', 'QIO' ),
        ( '.menu.FlashMode.qio.build.flash_mode', 'qio' ),
        ( '.menu.FlashMode.qout', 'QOUT' ),
        ( '.menu.FlashMode.qout.build.flash_mode', 'qout' ),
        ( '.menu.FlashMode.dio', 'DIO' ),
        ( '.menu.FlashMode.dio.build.flash_mode', 'dio' ),
        ( '.menu.FlashMode.dout', 'DOUT' ),
        ( '.menu.FlashMode.dout.build.flash_mode', 'dout' ),
        ]),

    ####################### default flash_mode
    
    'flashmode_dio': collections.OrderedDict([
        ( '.build.flash_mode', 'dio' ),
        ]),

    'flashmode_qio': collections.OrderedDict([
        ( '.build.flash_mode', 'qio' ),
        ]),

    'flashmode_dout': collections.OrderedDict([
        ( '.build.flash_mode', 'dout' ),
        ]),

    'flashmode_qout': collections.OrderedDict([
        ( '.build.flash_mode', 'qout' ),
        ]),

    ####################### lwip

    'lwip2': collections.OrderedDict([
        ( '.menu.LwIPVariant.open', 'v2' ),
        ( '.menu.LwIPVariant.open.build.lwip_include', 'lwip2/include' ),
        ( '.menu.LwIPVariant.open.build.lwip_lib', '-llwip2' ),
        ]),

    'lwip': collections.OrderedDict([
        ( '.menu.LwIPVariant.Prebuilt', 'v1.4 Prebuilt (gcc)' ),
        ( '.menu.LwIPVariant.Prebuilt.build.lwip_lib', '-llwip_gcc' ),
        ( '.menu.LwIPVariant.Prebuilt.build.lwip_flags', '-DLWIP_OPEN_SRC' ),
        ( '.menu.LwIPVariant.Espressif', 'v1.4 Espressif (xcc)' ),
        ( '.menu.LwIPVariant.Espressif.build.lwip_lib', '-llwip' ),
        ( '.menu.LwIPVariant.Espressif.build.lwip_flags', '-DLWIP_MAYBE_XCC' ),
        ( '.menu.LwIPVariant.OpenSource', 'v1.4 Open Source (gcc)' ),
        ( '.menu.LwIPVariant.OpenSource.build.lwip_lib', '-llwip_src' ),
        ( '.menu.LwIPVariant.OpenSource.build.lwip_flags', '-DLWIP_OPEN_SRC' ),
        ( '.menu.LwIPVariant.OpenSource.recipe.hooks.sketch.prebuild.1.pattern', 'make -C "{runtime.platform.path}/tools/sdk/lwip/src" install TOOLS_PATH="{runtime.tools.xtensa-lx106-elf-gcc.path}/bin/xtensa-lx106-elf-"' ),
        ]),

    ####################### serial

    's9': collections.OrderedDict([
        ( '.menu.UploadSpeed.9600', '9600' ),
        ( '.menu.UploadSpeed.9600.upload.speed', '9600' ),
        ]),
    's57': collections.OrderedDict([
        ( '.menu.UploadSpeed.57600', '57600' ),
        ( '.menu.UploadSpeed.57600.upload.speed', '57600' ),
        ]),
    's115': collections.OrderedDict([
        ( '.menu.UploadSpeed.115200', '115200' ),
        ( '.menu.UploadSpeed.115200.upload.speed', '115200' ),
        ]),
    's256': collections.OrderedDict([
        ( '.menu.UploadSpeed.256000.windows', '256000' ),
        ( '.menu.UploadSpeed.256000.upload.speed', '256000' ),
        ]),
    's230': collections.OrderedDict([
        ( '.menu.UploadSpeed.230400.linux', '230400' ),
        ( '.menu.UploadSpeed.230400.macosx', '230400' ),
        ( '.menu.UploadSpeed.230400.upload.speed', '230400' ),
        ]),
    's460': collections.OrderedDict([
        ( '.menu.UploadSpeed.460800.linux', '460800' ),
        ( '.menu.UploadSpeed.460800.macosx', '460800' ),
        ( '.menu.UploadSpeed.460800.upload.speed', '460800' ),
        ]),
    's512': collections.OrderedDict([
        ( '.menu.UploadSpeed.512000.windows', '512000' ),
        ( '.menu.UploadSpeed.512000.upload.speed', '512000' ),
        ]),
    's921': collections.OrderedDict([
        ( '.menu.UploadSpeed.921600', '921600' ),
        ( '.menu.UploadSpeed.921600.upload.speed', '921600' ),
        ]),

    }

################################################################
# defs

################################################################
# debug options

# https://rosettacode.org/wiki/Combinations#Python
def comb (m, lst):
    if m == 0: return [[]]
    return [[x] + suffix for i, x in enumerate(lst) for suffix in comb(m - 1, lst[i + 1:])]

def combn (lst):
    all = []
    for i in range(0, len(lst)):
        all += comb(i + 1, lst)
    return all

def comb1 (lst):
    all = []
    for i in range(0, len(lst)):
        all += [ [ lst[i] ] ]
    all += [ lst ]
    return all

def all_debug ():
    listcomb = [ 'SSL', 'TLS_MEM', 'HTTP_CLIENT', 'HTTP_SERVER' ]
    listnocomb = [ 'CORE', 'WIFI', 'HTTP_UPDATE', 'UPDATER', 'OTA' ]
    if not premerge:
        listnocomb += [ 'NULL -include "umm_malloc/umm_malloc_cfg.h"' ]
    options = combn(listcomb)
    options += comb1(listnocomb)
    options += [ listcomb + listnocomb ]
    debugmenu = collections.OrderedDict([
            ( '.menu.Debug.Disabled', 'Disabled' ),
            ( '.menu.Debug.Disabled.build.debug_port', '' ),
            ( '.menu.Debug.Serial', 'Serial' ),
            ( '.menu.Debug.Serial.build.debug_port', '-DDEBUG_ESP_PORT=Serial' ),
            ( '.menu.Debug.Serial1', 'Serial1' ),
            ( '.menu.Debug.Serial1.build.debug_port', '-DDEBUG_ESP_PORT=Serial1' ),
            ( '.menu.DebugLevel.None____', 'None' ),
            ( '.menu.DebugLevel.None____.build.debug_level', '' ),
        ])
    for optlist in options:
        debugname = ''
        debugmenuname = ''
        debugdefs = ''
        for opt in optlist:
            space = opt.find(" ")
            if space > 0:
                # remove subsequent associated gcc cmdline option
                simpleopt = opt[0:space]
            else:
                simpleopt = opt
            debugname += simpleopt
            if debugmenuname != '':
                debugmenuname += '+'
            debugmenuname += simpleopt
            debugdefs += ' -DDEBUG_ESP_' + opt
        debugmenu.update(collections.OrderedDict([
            ( '.menu.DebugLevel.' + debugname, debugmenuname ),
            ( '.menu.DebugLevel.' + debugname + '.build.debug_level', debugdefs )
            ]))
    return { 'debug_menu': debugmenu }

################################################################
# flash size

def flash_size (display, optname, ld, desc, max_upload_size, spiffs_start = 0, spiffs_size = 0, spiffs_blocksize = 0):
    menu = '.menu.FlashSize.' + optname
    menub = menu + '.build.'
    d = collections.OrderedDict([
        ( menu, display + ' (' + desc + ')' ),
        ( menub + 'flash_size', display ),
        ( menub + 'flash_ld', ld ),
        ( menub + 'spiffs_pagesize', '256' ),
        ( menu + '.upload.maximum_size', "%i" % max_upload_size ),
        ])
    if spiffs_start > 0:
        d.update(collections.OrderedDict([ 
            ( menub + 'spiffs_start', "0x%05X" % spiffs_start ),
            ( menub + 'spiffs_end', "0x%05X" % (spiffs_start + spiffs_size) ),
            ( menub + 'spiffs_blocksize', "%i" % spiffs_blocksize ),
            ]))

    if ldgen:
        if spiffs_size == 0:
            page = 0
            block = 0
        elif spiffs_size < 0x80000:
            page = 0x100
            block = 0x1000
        else:
            page = 0x100
            block = 0x2000
        print "/* file %s */" % ld
        print "/* Flash Split for %s chips */" % optname
        print "MEMORY"
        print "{"
        print "  dport0_0_seg :                        org = 0x3FF00000, len = 0x10"
        print "  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000"
        print "  iram1_0_seg :                         org = 0x40100000, len = 0x8000"
        print "  irom0_0_seg :                         org = 0x40201010, len = 0x%x" % max_upload_size
        print "}"
        print "PROVIDE ( _SPIFFS_start = 0x%08X );" % (0x40200000 + spiffs_start)
        print "PROVIDE ( _SPIFFS_end = 0x%08X );" % (0x40200000 + spiffs_start + spiffs_size)
        print "PROVIDE ( _SPIFFS_page = 0x%X );" % page
        print "PROVIDE ( _SPIFFS_block = 0x%X );" % block
        print ""
        print 'INCLUDE "../ld/eagle.app.v6.common.ld"'

    return d

def all_flash_size ():
    f512 =      flash_size('512K', '512K0',   'eagle.flash.512k0.ld',     'no SPIFFS', 499696)
    f512.update(flash_size('512K', '512K64',  'eagle.flash.512k64.ld',   '64K SPIFFS', 434160,   0x6B000,   0x10000, 4096))
    f512.update(flash_size('512K', '512K128', 'eagle.flash.512k128.ld', '128K SPIFFS', 368624,   0x5B000,   0x20000, 4096))
    f1m =       flash_size(  '1M', '1M0',     'eagle.flash.1m0.ld',       'no SPIFFS', 1023984)
    f1m.update( flash_size(  '1M', '1M64',    'eagle.flash.1m64.ld',     '64K SPIFFS', 958448,   0xEB000,   0x10000, 4096))
    f1m.update( flash_size(  '1M', '1M128',   'eagle.flash.1m128.ld',   '128K SPIFFS', 892912,   0xDB000,   0x20000, 4096))
    f1m.update( flash_size(  '1M', '1M144',   'eagle.flash.1m144.ld',   '144K SPIFFS', 876528,   0xD7000,   0x24000, 4096))
    f1m.update( flash_size(  '1M', '1M160',   'eagle.flash.1m160.ld',   '160K SPIFFS', 860144,   0xD3000,   0x28000, 4096))
    f1m.update( flash_size(  '1M', '1M192',   'eagle.flash.1m192.ld',   '192K SPIFFS', 827376,   0xCB000,   0x30000, 4096))
    f1m.update( flash_size(  '1M', '1M256',   'eagle.flash.1m256.ld',   '256K SPIFFS', 761840,   0xBB000,   0x40000, 4096))
    f1m.update( flash_size(  '1M', '1M512',   'eagle.flash.1m512.ld',   '512K SPIFFS', 499696,   0x7B000,   0x80000, 8192))
    f2m =       flash_size(  '2M', '2M',      'eagle.flash.2m.ld',        '1M SPIFFS', 1044464, 0x100000,   0xFB000, 8192)
    f4m =       flash_size(  '4M', '4M1M',    'eagle.flash.4m1m.ld',      '1M SPIFFS', 1044464, 0x300000,   0xFB000, 8192)
    f4m.update( flash_size(  '4M', '4M3M',    'eagle.flash.4m.ld',        '3M SPIFFS', 1044464, 0x100000,  0x2FB000, 8192))
    f8m =       flash_size(  '8M', '8M7M',    'eagle.flash.8m.ld',        '7M SPIFFS', 1044464, 0x100000,  0x6FB000, 8192)
    f16m =      flash_size( '16M', '16M15M',  'eagle.flash.16m.ld',      '15M SPIFFS', 1044464, 0x100000, 0x16FB000, 8192)
    return {
        '512K': f512,
          '1M':  f1m,
          '2M':  f2m,
          '4M':  f4m,
          '8M':  f8m,
         '16M': f16m
        }

################################################################
# builtin led

def led (default,max):
    led = collections.OrderedDict([
                ('.menu.led.' + str(default), str(default)),
                ('.menu.led.' + str(default) + '.build.led', '-DUSERLED=' + str(default)),
          ]);
    for i in range(0,max):
        if not i == default:
            led.update(
                collections.OrderedDict([
                    ('.menu.led.' + str(i), str(i)),
                    ('.menu.led.' + str(i) + '.build.led', '-DUSERLED=' + str(i)),
                ]))
    return { 'led': led }

################################################################
# help / usage

def usage (name,ret):
    print ""
    print "boards.txt generator for esp8266/Arduino"
    print ""
    print "usage: %s [options]" % name
    print ""
    print "	-h, --help"
    print "	--lwip			- preferred default lwIP version (default %d)" % lwip
    print "	--led			- preferred default builtin led for generic boards (default %d)" % led_default
    print "	--board b		- board to modify:"
    print "		--speed	s	- change default serial speed"
    print "	--premerge		- no NULL debug option, no led menu"
    print "	--customspeed s		- new serial speed for all boards"
    print ""
    print "	--boardsshow"
    print "	--boardsgen"
    print "	--ldshow"
    print "	--ldgen"
    print ""

    out = ""
    for s in speeds:
        out += s + ' '
    print "available serial speed options (kbps):", out

    out = ""
    for b in boards:
        out += b + '('
        if 'serial' in boards[b]:
            out += boards[b]['serial']
        else:
            out += default_speed
        out += 'k) '
    print "available board names:", out

    print ""

    sys.exit(ret)

################################################################
################################################################
# entry point

lwip = 2
default_speed = '115'
led_default = 2
led_max = 16
premerge = False
ldgen = False
ldshow = False
boardsgen = False
boardsshow = False
customspeeds = []

#### vvvv cmdline parsing starts

try:
    opts, args = getopt.getopt(sys.argv[1:], "h",
        [ "help", "premerge", "lwip=", "led=", "speed=", "board=", "customspeed=",
          "ldshow", "ldgen", "boardsshow", "boardsgen" ])
except getopt.GetoptError as err:
    print str(err)  # will print something like "option -a not recognized"
    usage(sys.argv[0], 1)

no = '(not set)'
board = no

for o, a in opts:

    if o in ("-h", "--help"):
        usage(sys.argv[0], 0)
    
    elif o in ("--premerge"):
        premerge = True

    elif o in ("--lwip"):
        lwip = a
    
    elif o in ("--led"):
        led_default = int(a)

    elif o in ("--customspeed"):
        customspeeds += [
            '.menu.UploadSpeed.' + a + '=' + a,
            '.menu.UploadSpeed.' + a + '.upload.speed' + '=' + a ]

    elif o in ("--board"):
        if not a in boards:
            print "board %s not available" % a
            usage(sys.argv[0], 1)
        board = a

    elif o in ("--speed"):
        if board == no:
            print "board not set"
            usage(sys.argv[0], 1)
        if not a in speeds:
            print "speed %s not available" % a
            usage(sys.argv[0], 1)
        boards[board]['serial'] = a

    elif o in ("--ldshow"):
        ldshow = True

    elif o in ("--ldgen"):
        ldshow = True
        ldgen = True

    elif o in ("--boardsshow"):
        boardsshow = True

    elif o in ("--boardsgen"):
        boardsshow = True
        boardsgen = True

    else:
        assert False, "unhandled option"

#### ^^^^ cmdline parsing ends

if ldshow:
    all_flash_size()

if not boardsshow:
    if not ldshow:
        usage(sys.argv[0], 0)
    sys.exit(0)

# board show / gen

if boardsgen:
    # check if running in root
    if not os.path.isfile("boards.txt"):
        print "please run me from boards.txt directory"
        sys.exit(1)

    # check if backup already exists
    if not os.path.isfile("boards.txt.orig"):
        os.rename("boards.txt", "boards.txt.orig")

    sys.stdout = open("boards.txt", 'w')

macros.update(all_flash_size())
macros.update(all_debug())
if premerge:
    macros.update({ 'led': { } })
else:
    macros.update(led(led_default, led_max))

print '#'
print '# this file is script-generated and is likely to be overwritten by ' + sys.argv[0]
print '#'
print ''
print 'menu.BoardModel=Model'
print 'menu.UploadSpeed=Upload Speed'
print 'menu.CpuFrequency=CPU Frequency'
print 'menu.CrystalFreq=Crystal Frequency'
print 'menu.FlashSize=Flash Size'
print 'menu.FlashMode=Flash Mode'
print 'menu.FlashFreq=Flash Frequency'
print 'menu.ResetMethod=Reset Method'
print 'menu.ESPModule=Module'
print 'menu.Debug=Debug port'
print 'menu.DebugLevel=Debug Level'
print 'menu.LwIPVariant=lwIP Variant'
print 'menu.led=Builtin Led'
print ''

for id in boards:
    print '##############################################################'
    board = boards[id]
    print id + '.name=' + board['name']

    # standalone options
    if 'opts' in board:
        for optname in board['opts']:
            print id + optname + '=' + board['opts'][optname]

    # macros
    macrolist = [ 'defaults', 'cpufreq_menu', ]
    if 'macro' in board:
        macrolist += board['macro']
    if lwip == 2:
        macrolist += [ 'lwip2', 'lwip' ]
    else:
        macrolist += [ 'lwip', 'lwip2' ]
    macrolist += [ 'debug_menu', ]

    for cs in customspeeds:
        print id + cs

    if 'serial' in board:
        macrolist += speeds[board['serial']]
    else:
        macrolist += speeds[default_speed]

    for block in macrolist:
        for optname in macros[block]:
            if not ('opts' in board) or not (optname in board['opts']):
                print id + optname + '=' + macros[block][optname]

    print ''
