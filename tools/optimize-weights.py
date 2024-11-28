#!/usr/bin/env python3

# Auto-tunes player score weights.
#
# Example usage:
#
# % tools/optimize-weights.py player/output/release/player | tee tmp/optimize-weights-output.txt
#

import concurrent.futures
from random import randint
import sys

from arbiter import RunGames

INITIAL_WEIGHTS = (250, 2500, 100, 1000, 10, 100, 1, 10)

PLAYER_COUNT = 10
ROUNDS = 10
THREADS = 4

assert PLAYER_COUNT % 2 == 0


def PerturbWeight(weight):
    delta = max(weight // 10, 1)
    weight = weight + randint(-1, 1) * delta
    return max(weight, 0)


def PerturbWeights(weights):
    return tuple(PerturbWeight(w) for w in weights)


def Main(args):
    out = sys.stdout
    executor = concurrent.futures.ThreadPoolExecutor(max_workers=4)
    player_weights = tuple(PerturbWeights(INITIAL_WEIGHTS) for _ in range(PLAYER_COUNT))
    names = [f'player-{i}' for i in range(PLAYER_COUNT)]
    rounds = 0
    while True:
        commands = [' '.join(args + ['--score-weights=' + ','.join(map(str, weights))]) for weights in player_weights]
        scores = RunGames(commands, names, ROUNDS, logdir=None, executor=executor)

        _, sorted_weights = zip(*sorted(zip(scores, player_weights), reverse=True))
        survivor_weights = sorted_weights[:PLAYER_COUNT // 2]
        player_weights = survivor_weights + tuple(PerturbWeights(weights) for weights in survivor_weights)
        rounds += 1

        print(f'Standings after {rounds} rounds', file=out)
        for i, weights in enumerate(sorted_weights):
            print(f'{i + 1}: {weights}', file=out)
        print(file=out)
        out.flush()


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f'Usage: {sys.argv[0]} <command> [<args...>]', file=sys.stderr)
        sys.exit(1)
    Main(sys.argv[1:])
