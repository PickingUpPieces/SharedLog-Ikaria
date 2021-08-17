#!/usr/bin/env bash

ipServer = fe80::9e69:b4ff:fe60:b86d 
interface=fe80::9e69:b4ff:fe60:b86d%enp2s0f1 

for SIZE in 64 256 4096
do
	echo "Testing with size $SIZE"

	iperf3 -c $ipServer -B $interface -p 5201 -t 100 -l $SIZE -i 10 --repeating-payload >> result_client1 &
	iperf3 -c $ipServer -B $interface -p 5202 -t 100 -l $SIZE -i 10 --repeating-payload >> result_client2 &
 	iperf3 -c $ipServer -B $interface -p 5203 -t 100 -l $SIZE -i 10 --repeating-payload >> result_client3 &
 	iperf3 -c $ipServer -B $interface -p 5204 -t 100 -l $SIZE -i 10 --repeating-payload >> result_client4 &
	wait
done
