#echo "SkipTrieHash no counters"
#for scalp in 4 8 12 16 20
#do
#    ./skip_hash_test.x 80 $scalp
#    #echo "./../sl_test.x $i"
#done

echo "SkipTrieHash running time"
for scalp in 4 8 12 16 20
do
    echo "SkipTrieHash scalp = $scalp"
    for i in 1 10 20 30 40 50 60 70 80
    do
	./skip_hash_test.x $i $scalp
    done
done

echo "SkipTrieHash perf stats"
#10,12,14
for scalp in 4 8 12 16 20
do
    echo "SkipTrieHash scalp = $scalp"
    for i in 1 10 20 30 40 50 60 70 80
    do
	time perf stat -e cpu-cycles -e stalled-cycles-frontend \
	    -e stalled-cycles-backend \
	    -e instructions          \
	    -e cache-references      \
	    -e cache-misses          \
	    -e branch-instructions  \
	    -e branch-misses                  \
	    -e bus-cycles                     \
	    -e cpu-clock                      \
	    -e task-clock                     \
	    -e page-faults           \
	    -e minor-faults                   \
	    -e major-faults                   \
	    -e context-switches          \
	    -e cpu-migrations   \
	    -e alignment-faults               \
	    -e emulation-faults               \
	    -e L1-dcache-loads                \
	    -e L1-dcache-load-misses          \
	    -e L1-dcache-stores               \
	    -e L1-dcache-store-misses         \
	    -e L1-dcache-prefetches           \
	    -e L1-dcache-prefetch-misses      \
	    -e L1-icache-loads                \
	    -e L1-icache-load-misses          \
	    -e L1-icache-prefetches           \
	    -e L1-icache-prefetch-misses      \
	    -e LLC-loads                      \
	    -e LLC-load-misses                \
	    -e LLC-stores                     \
	    -e LLC-store-misses               \
	    -e LLC-prefetches                 \
	    -e LLC-prefetch-misses            \
	    -e dTLB-loads                     \
	    -e dTLB-load-misses               \
	    -e dTLB-stores                    \
	    -e dTLB-store-misses              \
	    -e dTLB-prefetches \
	    -e dTLB-prefetch-misses           \
	    -e iTLB-loads                     \
	    -e iTLB-load-misses               \
	    -e branch-loads                   \
	    -e branch-load-misses             \
	    -e node-loads                     \
	    -e node-load-misses               \
	    -e node-stores                    \
	    -e node-store-misses              \
	    -e node-prefetches                \
	    -e node-prefetch-misses \
	    ./skip_hash_test.x $i $scalp

	#echo "./../strie_test.x $i $scalp"
    done
done