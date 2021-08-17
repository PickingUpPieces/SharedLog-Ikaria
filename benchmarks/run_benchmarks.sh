#!/usr/bin/env bash

nodeID=$1
runTime=5
sleepTime=$(expr $runTime + 5)
size=256
threads=(1 2 4 8 16)
readProb=(50 70 90)
current_time=$(date "+%d-%H.%M.%S")
fileName=$current_time"-"$nodeID"-benchmark.csv"

echo "NodeID: " $nodeID " RunTime: " $runTime " Value Size: " $size

for r in ${readProb[@]}; do
    for t in ${threads[@]}; do
        echo "Current run: Read Prob: " $r " Threads: " $t 

        sudo ./../benchmark -i $nodeID -r $r -t $t -h $runTime -f $fileName
        sleep $sleepTime
    done
done
