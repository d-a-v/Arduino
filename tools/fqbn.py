#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import re
import os
import sys
import json
import argparse
import collections

# tweaks per historically malformed entries
hide = ('ESPModule', 'BoardModel')

desc = 'Arduino boards.txt parser for FQBN details'

parser = argparse.ArgumentParser(description=desc)
parser.add_argument('--machine', action='store_true',
                    help='machine version')
args = parser.parse_args()

if not args.machine:
  print('#')
  print('# -- ' + desc + ' --')
  print('#')
  print('# random example:')
  print('#     esp8266com:esp8266:d1:xtal=80,vt=flash,exception=disabled,eesz=4M,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=460800')
  print('#')
  print('# description of possible values in configurable menu entries:')
  print('# (run with \'--machine\' for machine version)')
  print('#')
  print('#')

REmenuentry = re.compile('^([^\.]*)\.menu\.([^\.]*)\.([^\.]*)(?:\.linux|\.macos|\.windows|)=(.*)')
REmenudesc = re.compile('^menu\.([^\.]*)=(.*)')
REmenuinternal = re.compile('^([^\.]*)\.([^\.]*)\.([^\.]*)=(.*)')

menudescs = collections.OrderedDict([ ( 'UploadTool', 'Upload Tool' ) ])
menuentries = collections.OrderedDict([ ])
boards = collections.OrderedDict([ ])

boardstxt = False
for f in ( "../boards.txt", "boards.txt" ):
  if os.path.isfile(f):
    boardstxt = open(f)
if not boardstxt:
  print("cannot open " + f)
  sys.exit(1)

line = boardstxt.readline()

while line:

  catch = REmenudesc.match(line)
  if catch:
    #print("menudesc " + str(catch.groups()))
    menudescs.update(collections.OrderedDict([ ( catch.group(1), catch.group(2) ) ]))

  catch = REmenuentry.match(line)
  if catch:
    #print("menuentry " + str(catch.groups()))
    if not catch.group(2) in menuentries:
      menuentries.update(collections.OrderedDict([(catch.group(2), collections.OrderedDict([]))]))
    menuentries[catch.group(2)].update(collections.OrderedDict([(catch.group(3), catch.group(4))]))

  catch = REmenuinternal.match(line)
  if catch:
     #print("menuinternal " + str(catch.groups()))
     if catch.group(3) == 'board':
       boards.update(collections.OrderedDict([ (catch.group(1), catch.group(4)) ]))

  line = boardstxt.readline()


for p in menuentries:
  for q in menuentries[p]:
    menuentries[p][q] = menudescs[p] + ': ' + menuentries[p][q]

for h in hide:
  del menuentries[h]

all = collections.OrderedDict([
  ( 'boards', boards ),
  ( 'options', menudescs ),
  ( 'values', menuentries )
])

if args.machine:
  print(json.dumps(all))
else:
  print(json.dumps(all, indent=2))
  print()
