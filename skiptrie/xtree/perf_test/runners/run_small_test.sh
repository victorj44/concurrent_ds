echo "***************************"
echo "XTrie Big Test with variable # of threads"
for threads in 1 10 20 30 40 50 60 70 80
do
    echo "*********** $threads"
    ./perf_test $threads 100000
done

