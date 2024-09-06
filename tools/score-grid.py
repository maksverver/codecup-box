#!/usr/bin/env python3

import sys

import box

def StripComment(s):
    i = s.find('#')
    if i >= 0: s = s[:i]
    return s


def Main():
    if len(sys.argv) > 2:
        print('Usage: %s [<transcript>]' % (sys.argv[0],))
        sys.exit(1)

    with (open(sys.argv[1], 'rt') if len(sys.argv) == 2 else sys.stdin) as file:
        secret_colors = [int(word) for word in file.readline().split()]
        assert len(secret_colors) == 2
        board = None
        for line in file:
            line = StripComment(line).strip()
            if line == '': continue
            tile, placement = box.ParseTilePlacement(line)
            if board is None:
                board = box.BoardState(tile, placement)
            else:
                board.Place(tile, placement)

        for row in board.grid:
            print(*row)

        print(board.CalculateScores(secret_colors))

if __name__ == '__main__':
    Main()
