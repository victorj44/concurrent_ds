#!/bin/bash

for threads in 10 40 80
do
    echo "***************************"
    echo "SkipList with $threads threads"
    for size in 100 500 1000 2000 4000 8000 16000 32000 64000 128000 256000 512000
    do
	echo "***********"
	./perf_test $threads $size
    done
done
