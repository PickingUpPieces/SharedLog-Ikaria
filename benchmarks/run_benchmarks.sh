#!/usr/bin/env bash

chainLength=3
nodeID=$1
runTime=25
sleepTime=5
size=256
#threads=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
threads=(1 2 4 6 8 10 12 14 16)
readProb=(50 90)
current_time=$(date "+%d.%H")

echo "NodeID: " $nodeID " RunTime: " $runTime " Value Size: " $size

for r in ${readProb[@]}; do
    fileName=$chainLength"-"$r"-"$nodeID"-"$current_time"-benchmark.csv"
    for t in ${threads[@]}; do
        echo "Current run: Read Prob: " $r " Threads: " $t 

        sudo ./../benchmark -i $nodeID -r $r -t $t -h $runTime -f $fileName
        pid=$!
        wait $pid
        sleep $sleepTime
    done
done
