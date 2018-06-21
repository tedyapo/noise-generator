#!/bin/bash

echo -n > perf.dat
for t in `seq 1 4`; do
    for b in `seq 10 22`; do
        echo $t $b
        echo -n $t $b " " >> perf.dat
        ./noisegen -t $t -b $b | dd bs=65536 count=16384 iflag=fullblock of=/dev/null 2>&1 | grep -Po '[^ ]+(?= .B/s)' >> perf.dat
    done
done
