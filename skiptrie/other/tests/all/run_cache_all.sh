#!/bin/bash

#1-80 threads of a regular skiplist
echo "SkipList"
for i in 1, 10, 20, 30, 40, 50, 60, 70, 80
do
    time perf stat -e cache-references -e cache-misses ./sl_test.x $i
    #echo "./../sl_test.x $i"
done

#10,12,14
for scalp in 10, 12, 14
do
    echo "SkipTrie scalp = $scalp"
    for i in 1, 10, 20, 30, 40, 50, 60, 70, 80
    do
	time perf stat -e cache-references -e cache-misses ./strie_test.x $i $scalp
	#echo "./../strie_test.x $i $scalp"
    done
done

