#makefile for SkipTrie

CC=g++
CFLAGS= -02 -Wall -gstabs
LDFLAGS=

#OBJ_FILES=LazySkipList/lsk.o

all:
	$(MAKE) -C xtree/perf_test
	$(MAKE) -C skip_list/perf_test
	$(MAKE) -C skip_trie/perf_test

no_papi:
	$(MAKE) -C xtree/perf_test -e no_papi
	$(MAKE) -C skip_list/perf_test -e no_papi
	$(MAKE) -C skip_trie/perf_test -e no_papi

clean:
	rm -r -f *.o *.x */*.x */*.o
