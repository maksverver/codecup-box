#!/bin/sh

# Replay a game using the player log to determine random seed and I/O.
# This is useful when debugging and for performance analysis.
#
# Note that the player command invocation must match exactly!

if [ $# -lt 2 ]; then
    echo "Usage: $0 <logfile> <command> [<args...>]"
    exit
fi

logfile=$1
command=$2
shift 2

seed=$(awk '$1 == "SEED" { print $2 }' "$logfile")

if ! diff \
        <(sed -n -e 's/^IO SEND \[\(.*\)\]/\1/p' "$logfile") \
        <(sed -n -e 's/^IO RCVD \[\(.*\)\]/\1/p' "$logfile" | $command --seed=$seed $@);
then
    # When this happens, it may mean the player binary or options were different.
    echo "WARNING! Player output did not match logged output."
    exit 1
fi
