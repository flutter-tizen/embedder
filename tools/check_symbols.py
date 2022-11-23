#!/usr/bin/env python3
# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import subprocess
import sys


class Symbol:

  def __init__(self, addr, type, name):
    self.addr = addr
    self.type = type
    self.name = name

  def __str__(self):
    return '{} {} {}'.format(self.addr, self.type, self.name)

  @staticmethod
  def parse(line):
    # Format: "         U abort@GLIBC_2.4" (addr can be empty)
    return Symbol(line[:8].strip(), line[9], line[11:].strip().split('@')[0])


def check_symbol(sofile, allowlist):
  if not os.access(sofile, os.R_OK):
    sys.exit('{} is not a valid file.'.format(sofile))
  if not os.access(allowlist, os.R_OK):
    sys.exit('{} is not a valid file.'.format(allowlist))

  try:
    symbols_raw = subprocess.check_output(
      ['nm', '-gDC', sofile]).decode('utf-8').splitlines()
    symbols = [Symbol.parse(line) for line in symbols_raw]
  except subprocess.CalledProcessError as error:
    sys.exit('nm failed: {}'.format(error))

  with open(allowlist, 'r') as file:
    allowlist = [line.strip() for line in file.readlines()]

  not_allowed = []
  for symbol in symbols:
    if symbol.addr:
      continue
    if symbol.name.startswith('FlutterEngine'):
      continue
    if symbol.name in allowlist:
      continue
    not_allowed.append(symbol)

  if not_allowed:
    print('Symbols not allowed ({}):'.format(sofile))
    for symbol in not_allowed:
      print(symbol)
    sys.exit(1)


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--allowlist', type=str, required=True,
                      help='Path to the allowlist file')
  parser.add_argument('sofile', type=str, nargs='+',
                      help='Path to the .so file')
  args = parser.parse_args()

  for sofile in args.sofile:
    check_symbol(sofile, args.allowlist)


# Execute only if run as a script.
if __name__ == '__main__':
  main()
