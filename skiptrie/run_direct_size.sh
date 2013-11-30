#this script runs xtree with small sizes
#designed to compare perf of the xtrie (with a regular hash table)
#and the direct xtrie (with just a random access array)

if [ "$1" != "xtree" ];
then
    echo "wrong structure name (xtree only)"
    exit 1
fi

#rm output.txt
echo "$1" >> output.txt
path="$1/perf_test/direct_test.x"

echo "***************************"
echo "$1 1 thread variable size"
#L2 = 8MB
#L3 = 30MB
#sizes 5k, 10k, 20k, 100k, 500k, 1M, 2M, 4M, 8M, 16M = # of keys
for size in 5000 10000 20000 100000 500000 1000000 2000000 4000000 8000000
do
    echo "*********** $size"
    ./$path 1 $size
done
