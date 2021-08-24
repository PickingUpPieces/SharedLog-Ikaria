#!/usr/bin/env bash

nodeID=$1
chainLength=$2
numberOfRuns=(1 2 3)
#inFlightCap=(1000) 
inFlightCap=(100 1000 5000 10000) 
runTime=20
sleepTime=2
#size=(256)
#size=(64 256 1024 2048)
size=(256)
threads=(8)
#threads=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
readProb=(50 80 90)

echo "NodeID: " $nodeID " RunTime: " $runTime " Value Size: " $size

for a in ${numberOfRuns[@]}; do
    fileName=$nodeID"-"$a"-benchmark.csv"
    for r in ${readProb[@]}; do
        for i in ${inFlightCap[@]}; do
            for t in ${threads[@]}; do
                for s in ${size[@]}; do
                    echo "Current run: Run: " $a " Read Prob: " $r " Threads: " $t " Size: " $s " messagesInFlightCap: " $i
        
                    sudo ./../benchmark -i $nodeID -r $r -t $t -h $runTime -f $fileName -s $s -j $i -c $chainLength
                    pid=$!
                    wait $pid
                    sleep $sleepTime
                done
            done
        done
    done
done

sudo rm /dev/shm/replNode-$nodeID.log
