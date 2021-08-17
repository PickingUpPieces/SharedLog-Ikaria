#!/bin/bash

nodeID=$1
runTime=60
sleepTime=$(expr $runTime + 5)
size=256
threads=(1 2 4 8 16)
readProb=(50 70 90)
# TODO: Generate csv filename: timestamp-benchmark.csv 

echo $nodeID " " $runTime " " $size

for r in ${readProb[@]}; do
    for t in ${threads[@]}; do
        sudo ./../benchmark -i $nodeID -r $r -t $t -h $runTime
        sleep $sleepTime
    done
done
