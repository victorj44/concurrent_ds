#!/bin/bash

for threads in 10 40 80
do
    echo "***************************"
    echo "X-Trie with $threads threads"
    for size in 100 500 1000 2000 4000 8000 16000 32000 64000 128000 256000 512000 1024000 2048000 4000000
    do
	echo "***********"
	time perf stat \
	    -e cpu-cycles \
	    -e instructions \
	    -e cache-references \
	    -e cache-misses \
	    -e L1-dcache-loads \
	    -e L1-dcache-load-misses \
	    -e LLC-loads \
	    -e LLC-load-misses \
	    ./perf_test $threads $size
    done
done


