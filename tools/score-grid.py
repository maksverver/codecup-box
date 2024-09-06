#!/usr/bin/env python3

import sys

import box

# Returns a generator that removes comments from lines (starting with '#'),
# strips trailing whitespace, and yields only the nonempty resulting lines.
def StripComments(file):
    for line in file:
        i = line.find('#')
        if i >= 0: line = line[:i]
        line = line.rstrip()
        if line != '': yield line

def Main():
    if len(sys.argv) > 2:
        print('Usage: %s [<transcript>]' % (sys.argv[0],))
        sys.exit(1)

    with (open(sys.argv[1], 'rt') if len(sys.argv) == 2 else sys.stdin) as file:
        lines = StripComments(file)
        secret_colors = list(map(int, next(lines).split()))
        assert len(secret_colors) == 2
        board = None
        for line in lines:
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
