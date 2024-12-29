#!/usr/bin/env python3

# Script to split a competition into independent double round-robin tournaments.
#
# In the official CodeCup final, each user will play each other user 10 times,
# 5 times with different tilesets, and for each tileset 2 times (once as first
# and once as second player).
#
# This script splits the competition results into five independent competitions
# based on a single round-robin tournament. The point is to demonstrate the
# difference in ranking between individual tournaments and therefore the value
# of playing multiple rounds.
#
# The script assumes that if there are 5 matchups between each pair of players,
# they occur in 5 consecutive rounds, so we can use (round % 5) to separate the
# rounds.
#
# This script shares some common code with top-10.py but I'm too lazy to
# unify the common code.

from collections import defaultdict
import csv

# Number of rounds played per pair of players.
K = 5

total_score_by_user = defaultdict(int)
score_by_user_split = [defaultdict(int) for _ in range(K)]

def AddScore(round, user, score):
    total_score_by_user[user] += score
    score_by_user_split[round % K][user] += score

def Process(record):
    if record['IsSwiss'] != 'final': return
    AddScore(int(record['Round']), record['User1'], int(record['Score1']))
    AddScore(int(record['Round']), record['User2'], int(record['Score2']))


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
            assert False  # does this ever happen?
        else:
            last_score = score
            last_rank = rank
        result.append((rank, score, user))
    return result

def Main():
    if len(sys.argv) != 2:
        print(f'Usage: {sys.argv[0]} <transcripts.csv>')
        sys.exit(1)
    transcripts_filename = sys.argv[1]

    with open(transcripts_filename, newline='') as csvfile:
        for i, row in enumerate(csv.reader(csvfile)):
            if i == 0:
                header = row
            else:
                record = dict(zip(header, row))
                Process(record)

    ranks_by_user = defaultdict(list)
    scores_by_user = defaultdict(list)
    for split_scores in score_by_user_split:
        for rank, score, user in RankUsers(split_scores):
            ranks_by_user[user].append(rank)
            scores_by_user[user].append(score)

    print('Rank     TotScore AvgRank  MinRank  MaxRank  RankDiff MinScore MaxScore ScoreDif ScoreDi% Contestant')
    print(*['-'*8]*10, '-'*20)
    for rank, score, user in RankUsers(total_score_by_user):
        min_rank = min(ranks_by_user[user])
        max_rank = max(ranks_by_user[user])
        avg_rank = sum(ranks_by_user[user]) / len(ranks_by_user[user])
        min_score = min(scores_by_user[user])
        max_score = max(scores_by_user[user])
        print(f'{rank:>8} {score:>8} {avg_rank:>8.2f} {min_rank:>8} {max_rank:>8} {max_rank-min_rank:>8} {min_score:>8} {max_score:>8} {max_score-min_score:>8} {100.0*(max_score-min_score)/min_score:>7.2f}% {user}')
    print(*['-'*8]*10, '-'*20)

if __name__ == '__main__':
    Main()
