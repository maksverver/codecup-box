#!/usr/bin/env python3

# Script to calculate the scores among the top 10 players only.
#
# Example usage:
#
#  tools/top-10.py competition-results/test-6-competition-328-transcripts.csv
#
# NewRank  OldRank  Score    Contestant
# -------- -------- -------- --------------------
#        1        2    15451 Mark ter Brugge
#        2        1    15252 Maks Verver
#        3        5    14400 Leopold Tschanter
#        4        4    14163 Tapani Utriainen
#        5        6    13799 Niklas Een
#        6        3    13502 Jan Nunnink
#        7        7    13020 Matthias Gelbmann
#        8        8    12467 Pardhav Maradani
#        9        9    11516 Alvaro Carrera
#       10       10    11430 Louis Verhaard
# -------- -------- -------- --------------------
#
# The purpose is to evaluate whether rankings change by excluding lower-ranked
# players; some players may have won because they got more points from
# low-ranking players rather than winning more often against high-ranking
# players.
#
# Note that rankings among the top 10 are more variable because the number
# of matches per player pair is very low (currently 10), so a change in ranking
# might not actually be statistically significant.
#
# This script shares some common code with split-rounds.py but I'm too lazy to
# unify the common code.

from collections import defaultdict
import csv
import sys

# Number of players to keep in the top N.
TOP_N = 10

def RankUsers(scores_by_user):
    result = []
    last_score = None
    last_rank = 0
    for rank, (score, user) in enumerate(
            sorted(
                ((score, user) for (user, score) in scores_by_user.items()),
                reverse=True),
            start=1):
        if score == last_score:
            rank = last_rank
        else:
            last_score = score
            last_rank = rank
        result.append((rank, score, user))
    return result


def CalculateScores(transcripts_filename, top_users=None):
    total_score_by_user = defaultdict(int)

    def AddScore(round, user, score):
        total_score_by_user[user] += score

    def Process(record):
        if (record['IsSwiss'] != 'final' or 
            top_users is not None and (
                record['User1'] not in top_users or
                record['User2'] not in top_users)):
            return
        AddScore(int(record['Round']), record['User1'], int(record['Score1']))
        AddScore(int(record['Round']), record['User2'], int(record['Score2']))

    with open(transcripts_filename, newline='') as csvfile:
        for i, row in enumerate(csv.reader(csvfile)):
            if i == 0:
                header = row
            else:
                record = dict(zip(header, row))
                Process(record)

    return total_score_by_user


def Main():
    if not (2 <= len(sys.argv) <= 3):
        print(f'Usage: {sys.argv[0]} <transcripts.csv> [<N>]')
        sys.exit(1)
    transcripts_filename = sys.argv[1]
    if len(sys.argv) > 2:
        TOP_N = int(sys.argv[2])

    # Calculate all scores to determine the top users
    scores_by_user = CalculateScores(transcripts_filename)
    top_users = [user for rank, score, user in sorted(RankUsers(scores_by_user)) if rank <= TOP_N]

    assert 2 <= TOP_N <= len(top_users)

    # Calculate the scores if only the top users participated
    scores_by_top_user = CalculateScores(transcripts_filename, top_users=top_users)

    # OldRank is the rank in the global competition
    # NewRank is the rank based on matches between top 10 players
    print('NewRank  OldRank  Score    Contestant')
    print(*['-'*8]*3, '-'*20)
    for rank, score, user in RankUsers(scores_by_top_user):
        print(f'{rank:>8} {top_users.index(user)+1:>8} {score:>8} {user}')
    print(*['-'*8]*3, '-'*20)

if __name__ == '__main__':
    Main()
