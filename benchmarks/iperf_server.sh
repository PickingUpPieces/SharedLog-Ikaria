#!/usr/bin/env bash

interface=fe80::9e69:b4ff:fe60:b86d%enp2s0f1 

iperf3 -s -B $interface -p 5201 -i 10 &
iperf3 -s -B $interface -p 5202 -i 10 &
iperf3 -s -B $interface -p 5203 -i 10 &
iperf3 -s -B $interface -p 5204 -i 10
