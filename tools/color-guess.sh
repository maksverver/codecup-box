#!/bin/bash

# Calculates how quickly each player's secret color can be guessed.

ANALYZER=player/output/release/analyzer
COMPETITION=test-3-competition-325

# Runs analyzer to calculate from point in the game each player's secret color
# can be accurately guessed.
#
# Then outputs lines like "User Name,7", where 7 is the number of the player's
# moves until the guess is accurate.
process_logs() {
  exec < "competition-results/$COMPETITION-transcripts.csv"
  read header
  while IFS=, read game round is_swiss user1 score1 status1 user2 score2 status2 color1 color2 moves
  do
    answers=($($ANALYZER --color1=$color1 --color2=$color2 $moves 2>&1 \
        | sed -E -n -e 's/Player [12] color [(][1-6][)] guessed last incorrect on move ([0-9]+)/\1/p'))
    if [ "${#answers[@]}" -ne 2 ]; then
      echo "Unexpected number of answers: ${#answers[@]} (expected 2)" >&2
    else
      echo "$user1,${answers[0]}"
      echo "$user2,${answers[1]}"
    fi
  done
}
# Aggregates by username, and prints the average.
process_logs | awk -F, '
{
  sum[$1] += $2;
  cnt[$1]++;
}
END {
  for (user in sum) {
    t=sum[user]
    n=cnt[user]
    printf("%6.3f %7.3f %2d %s\n", t/n, t, n, user)
  }
}
' | sort -n;
