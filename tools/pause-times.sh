#!/bin/bash

# Calculates my opponent's pause times from my player logs. Pause times indicate
# how long an opponent spends thinking during the game.

COMPETITION=$(basename $(ls competition-results/*-transcripts.csv | tail -1) -transcripts.csv)

echo "Analyzing $COMPETITION" >&2

#
# Extracts the "PAUSE <duration_ms> <total_ms>" lines from the corresponding
# player logs.
#
# Then outputs lines like "User Name,123", where 123 is the final pause time in
# milliseconds. Note that commas are used as delimiters since names usually
# contain spaces.
process_logs() {
  exec < "competition-results/$COMPETITION-transcripts.csv"
  read header
  while IFS=, read game round is_swiss user1 score1 status1 user2 score2 status2 color1 color2 moves
  do
    if [ "$user1" == "Maks Verver" ]; then
      echo -n "${user2},"
    elif [ "$user2" == "Maks Verver" ]; then
      echo -n "${user1},"
    else
      # Don't have a log for this game because I didn't play in it.
      continue
    fi
    awk '$1 == "PAUSE" { v = $3; }; END { print v }' "playerlogs/$COMPETITION/game-$game.txt"
  done
}

# Aggregates
process_logs | awk -F, '
{
  sum[$1] += $2;
  cnt[$1]++;
}
END {
  for (user in sum) {
    t=sum[user] / 1000.0
    n=cnt[user]
    printf("%6.3f %7.3f %2d %s\n", t/n, t, n, user)
  }
}
' | sort -n;
