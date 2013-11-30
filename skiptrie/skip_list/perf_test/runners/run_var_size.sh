echo "***************************"
echo "SkipList 1 thread variable size"
#up to 0.5*10^9 items
#limits in the test generator and the key size!
for size in 10000 20000 40000 80000 160000 320000 640000 1280000 2560000 5120000 10240000 20480000 40960000 81920000
do
    echo "*********** $size"
    ./perf_test 1 $size
done
