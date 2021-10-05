#!/usr/bin/env bash

nodeID=$1
chainLength=$2
numberOfRuns=(1)
inFlightCap=(25) 
runTime=20
sleepTime=5
size=(256)
#size=(64 256 1024 2048 4096)
threads=(16)
#threads=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
#readProb=(50 80 90)
readProb=(80 100)

sudo rm /dev/shm/replNode-$nodeID.log
sudo rm /dev/hugepages/* 
sudo rm *.csv

echo "NodeID: " $nodeID " RunTime: " $runTime " Value Size: " $size

for a in ${numberOfRuns[@]}; do
    fileName=$nodeID"-"$a"-benchmark.csv"
    for r in ${readProb[@]}; do
        for i in ${inFlightCap[@]}; do
            for t in ${threads[@]}; do
                for s in ${size[@]}; do
                    echo "Current run: Run: " $a " Read Prob: " $r " Threads: " $t " Size: " $s " messagesInFlightCap: " $i
        
                    sudo PMEM_IS_PMEM_FORCE=1 ./../benchmark -i $nodeID -r $r -t $t -h $runTime -f $fileName -s $s -j $i -c $chainLength
                    pid=$!
                    wait $pid
                    sleep $sleepTime
                done
            done
        done
    done
done

sudo rm /dev/shm/replNode-$nodeID.log
sudo rm /dev/hugepages/* 
