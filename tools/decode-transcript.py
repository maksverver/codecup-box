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
    if len(sys.argv) != 2:
      print('Usage: %s [<transcript>]' % (sys.argv[0],))
      sys.exit(1)

    _, s = sys.argv
    assert len(s) >= 4 and (len(s) - 1) % 3 == 0

    print(*coding.DecodeSecretColors(s[0]))
    for i in range(1, len(s), 3):
        tile, placement = coding.DecodeTilePlacement(s[i:i + 3])
        print(box.FormatTilePlacement(tile, placement))

if __name__ == '__main__':
    Main()
