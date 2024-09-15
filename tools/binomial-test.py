#!/usr/bin/env python3

# Performs a binomial test to calculate which version of a player is better.
#
# To use this, run a competition between exactly two players, then pass the
# number of wins to this script to calculate the probability that one player
# is better than the other. Example:
#
#   % tools/binomial-test.py 35 14
#   Player 1 is better than Player 2 (p=0.00190083)
#
# The p-value is the probability that the null-hypothesis holds, assuming
# that the players are equally matched, so lower p-values mean greater
# confidence in the claim that player 1 is better than player 2.
#
# This does a one-tailed test (i.e., it asks: what is the probability that
# the number of wins is at least as large as this?) (Maybe we should do a
# two-tailed test instead?)
#
# It also prints the 95% confidence interval of the true winrate (i.e.
# probability that player 1 defeats player 2, when they do not tie).

from functools import cache
import sys

@cache
def nCr(n, r):
    '''Computes the bionial coefficient n over r, which is the number of subsets
        of size `r` of a set of `n`.'''
    assert 0 <= r <= n
    return 1 if r == 0 or r == n else nCr(n - 1, r - 1) + nCr(n - 1, r)

def CalcProb(wins, games, win_prob):
    '''Calculates the probability of having at least `wins` wins out of a total
       of `games` games, assuming that the probability of winning a match is
       `win_prob`.'''
    assert 0 <= wins <= games
    return sum(nCr(games, w) * win_prob**w * (1 - win_prob)**(games - w)
        for w in range(wins, games + 1))

def BinarySearch(lo, hi, steps, test):
    for _ in range(steps):
        mid = lo + (hi - lo) / 2
        if test(mid):
            hi = mid
        else:
            lo = mid
    return mid

def CalcCI(wins, games, confidence=0.95):
    lo_prob = (1.0 - confidence) / 2  # e.g. 0.025
    hi_prob = lo_prob + confidence    # e.g. 0.975
    lo_bound = BinarySearch(0.0, 1.0, 100, lambda v: CalcProb(wins, games, v) >= lo_prob)
    hi_bound = BinarySearch(0.0, 1.0, 100, lambda v: CalcProb(wins, games, v) >= hi_prob)
    return (lo_bound, hi_bound)

def Main(wins1, wins2):
    confidence = 0.95
    name1, name2 = 'Player 1', 'Player 2'
    if wins1 == wins2:
        print('Players are tied')
    else:
        if wins1 < wins2:
            wins1, wins2 = wins2, wins1
            name1, name2 = name2, name1
        games = wins1 + wins2
        p = CalcProb(wins1, games, 0.5)
        CalcCI(wins1, games)
        ci_lo, ci_hi = CalcCI(wins1, games, confidence)
        print('%s is better than %s (p=%g, winrate=%f, n=%d, %d%% CI=[%.3f,%.3f])' %
            (name1, name2, p, wins1 / games, games, confidence * 100, ci_lo, ci_hi))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage', sys.argv[0], '<wins1>', '<wins2>')
        sys.exit(1)

    a, b = map(int, sys.argv[1:])
    Main(a, b)
