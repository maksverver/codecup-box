#!/usr/bin/env python3

import sys

import box
import coding
import utils

def Process(file):
    lines = utils.StripComments(file)
    secret_colors = list(map(int, next(lines).split()))
    assert len(secret_colors) == 2
    s = coding.EncodeSecretColors(secret_colors)
    for line in lines:
        tile, placement = box.ParseTilePlacement(line)
        s += coding.EncodeTilePlacement(tile, placement)
    print(s)

def Main():
    if len(sys.argv) < 2:
      Process(sys.stdin)
    else:
      for arg in sys.argv[1:]:
        with open(arg, 'rt') as file:
          Process(file)

if __name__ == '__main__':
    Main()
