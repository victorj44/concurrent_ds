echo "***************************"
echo "XTrie Big Test with variable # of threads"
for threads in 1 10 20 30 40 50 60 70 80
do
    echo "*********** $threads"
    time perf stat -e cache-references -e cache-misses ./perf_test $threads 10000000
done
