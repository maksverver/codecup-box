#!/bin/bash

# Calculates my opponent's pause times from my player logs. Pause times indicate
# how long an opponent spends thinking during the game.

COMPETITION=$(basename $(ls competition-results/*-transcripts.csv | tail -1) -transcripts.csv)
COMPETITION=final-competition-322

MY_USER='Maks Verver'

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
    if [ "$user1" == "$MY_USER" ]; then
      echo -n "${user2},"
    elif [ "$user2" == "$MY_USER" ]; then
      echo -n "${user1},"
    else
      # Don't have a log for this game because I didn't play in it.
      continue
    fi
    awk '$1 == "PAUSE" { v = $3; }; END { print v }' "playerlogs/$COMPETITION/game-$game.txt"

    echo -n "${MY_USER},"
    awk '$1 == "TIME"  { v = $3; }; END { print v }' "playerlogs/$COMPETITION/game-$game.txt"
  done
}

# Aggregates
process_logs | awk -F, '
{
  if (cnt[$1]) {
    cnt[$1]++;
    sum[$1] += $2;
    if ($2 < min[$1]) min[$1] = $2;
    if ($2 > max[$1]) max[$1] = $2;
  } else {
    cnt[$1] = 1;
    sum[$1] = $2;
    min[$1] = $2;
    max[$1] = $2;
  }
}
END {
  printf("%6s %6s %6s %9s %3s %s\n", "Avg", "Min", "Max", "Sum", "Cnt", "User")
  for (user in sum) {
    t=sum[user] / 1000.0
    n=cnt[user]
    mn=min[user] / 1000.0
    mx=max[user] / 1000.0
    printf("%6.3f %6.3f %6.3f %9.3f %3d %s\n", t/n, mn, mx, t, n, user)
  }
}
' | sort -n;
