#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <server_ip> <server_select_port_1>"
    exit 1
fi

SERVER_IP=$1

touch results.txt
> results.txt

touch results1.txt
> results1.txt

SERVER_PORT=$2

for ((c = 5; c<=75; c+=5)); do
    elapsed_time=0
    start=$(date +%s%N)
    for ((j = 1; j<=c; j++)); do
        ./client $SERVER_IP $SERVER_PORT &
    done
    wait
    end=$(date +%s%N)
    elapsed_time=$(($(($end-$start))/1000))
    elapsed_time=$(($elapsed_time/$c))
    echo $elapsed_time >> results.txt
done

for ((c = 5; c<=50; c+=5)); do
    elapsed_time=0
    start=$(date +%s%N)
    i=0
    while [ 1 ];
    do
        echo $elapsed_time
        if [ $elapsed_time -gt 50000 ]; then
            break
        fi
        for ((j = 1; j<=c; j++)); do
            ./client $SERVER_IP $SERVER_PORT &
        done
        wait
        i=$((i+1))
        end=$(date +%s%N)
        elapsed_time=$(($(($end-$start))/1000))
    done 
    i=$((i*20))
    echo $i >> results1.txt
done