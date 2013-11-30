for scalp in 0 2 4 6 8 10 12 14 16 18 20 22 24
do
    echo "SkipTrie scalp = $scalp"
    ./strie_test.x 80 $scalp
done