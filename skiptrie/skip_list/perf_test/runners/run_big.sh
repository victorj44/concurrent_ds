#!/bin/bash

echo "***************************"
echo "XTrie Big Test with 1 threads"
for size in 1000000 2000000 4000000 8000000 16000000 32000000
do
    echo "***********"
    numactl --cpunodebind=1 ./perf_test 1 $size
done

echo "***************************"
echo "XTrie Big Test with 10 threads"
for size in 1000000 2000000 4000000 8000000 16000000 32000000
do
    echo "***********"
    numactl --cpunodebind=1 ./perf_test 10 $size
done

echo "***************************"
echo "XTrie Big Test with 40 threads"
for size in 1000000 2000000 4000000 8000000 16000000 32000000
do
    echo "***********"
    numactl --cpunodebind=1,2 ./perf_test 40 $size
done

echo "***************************"
echo "XTrie Big Test with 80 threads"
for size in 1000000 2000000 4000000 8000000 16000000 32000000
do
    echo "***********"
    numactl --cpunodebind=0,1,2,3 ./perf_test 80 $size
done

