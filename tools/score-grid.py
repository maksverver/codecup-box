#!/usr/bin/env python3

import sys

import box
import utils

def Main():
    if len(sys.argv) > 2:
        print('Usage: %s [<transcript>]' % (sys.argv[0],))
        sys.exit(1)

    with (open(sys.argv[1], 'rt') if len(sys.argv) == 2 else sys.stdin) as file:
        lines = utils.StripComments(file)
        secret_colors = list(map(int, next(lines).split()))
        assert len(secret_colors) == 2
        board = None
        for line in lines:
            tile, placement = box.ParseTilePlacement(line)
            if board is None:
                board = box.BoardState(tile, placement)
            else:
                board.Place(tile, placement)

        if not sys.stdout.isatty():
            for row in board.grid:
                print(*row)
        else:
            RED  = "\033[1;41m"
            BLUE = "\033[1;44m"
            END  = "\033[0m"
            for row in board.grid:
                for i, val in enumerate(row):
                    if i > 0: sys.stdout.write(' ')
                    if val == secret_colors[0]:
                        sys.stdout.write(RED + str(val) + END)
                    elif val == secret_colors[1]:
                        sys.stdout.write(BLUE + str(val) + END)
                    else:
                        sys.stdout.write(str(val))
                sys.stdout.write('\n')

        print(board.CalculateScores(secret_colors))

if __name__ == '__main__':
    Main()
