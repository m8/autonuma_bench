#!/bin/bash
# pass your program as parameter
echo "$@"

sudo nice -n -20 ./nsampler.sh bench &
PLOT_PID=$!
echo $PLOT_PID
/usr/bin/time -f"%e" "$@"
sudo killall nsampler.sh