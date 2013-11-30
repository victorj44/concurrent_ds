#!/bin/bash

#3 reps each
for rep in {1..3}
do
    echo "rep $rep"
#1-80 threads of a regular skiplist
    echo "SkipList"
    for i in 1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80
    do
	./sl_test.x $i
	#echo "./../sl_test.x $i"
    done

#10,12,14
    for scalp in 0 2 4 6 8 10 12 14
    do
	echo "SkipTrie scalp = $scalp"
	for i in  1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80
	do
	    ./strie_test.x $i $scalp
	    #echo "./../strie_test.x $i $scalp"
	done
    done
done

