#!/usr/bin/env bash

numberOfRuns=$1
numberOfRuns=$(($numberOfRuns+1))

arg=""
i=1
while [ $i -ne $numberOfRuns ]
do
    arg=$arg" *-"$i"-benchmark.csv"
    echo "$arg"
    i=$(($i+1))
done

python add_up_csv.py -n "$arg"
python calculate_avg.py -n merged-0-*