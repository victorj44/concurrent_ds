if [ "$1" != "skip_list" -a "$1" != "xtree" -a "$1" != "skip_trie" ];
then
    echo "wrong structure name (skip_list/xtree)"
    exit 1
fi

#rm output.txt
echo "$1" >> output.txt
path="$1/perf_test/perf_test.x"

echo "***************************"
echo "$1 1 thread variable size"
#for size in 10000 20000 40000 80000 160000 320000 640000 1280000 2560000 5120000 10240000 20480000 40960000 81920000 163840000
for size in 5000 10000 20000 100000 500000 1000000 2000000 4000000 8000000
do
    echo "*********** $size"
    ./$path 1 $size
done
