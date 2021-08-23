#!/usr/bin/env bash

chainLength=$2
nodeID=$1
inFlightCap=1000 
runTime=20
sleepTime=5
size=(256)
#size=(64 256 1024 2048 4096)
threads=(16)
#threads=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
readProb=(50 80 90)

echo "NodeID: " $nodeID " RunTime: " $runTime " Value Size: " $size

for r in ${readProb[@]}; do
    fileName=$nodeID"-benchmark.csv"
    for t in ${threads[@]}; do
        for s in ${size[@]}; do
            echo "Current run: Read Prob: " $r " Threads: " $t " Size: " $s

            sudo ./../benchmark -i $nodeID -r $r -t $t -h $runTime -f $fileName -s $s -j $inFlightCap -c $chainLength
            pid=$!
            wait $pid
            sleep $sleepTime
        done
    done
done

sudo rm /dev/shm/replNode-$nodeID.log
