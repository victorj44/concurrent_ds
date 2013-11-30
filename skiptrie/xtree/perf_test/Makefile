#makefile for concurrent tests

CC=g++
CFLAGS= -Wall -g -O2 -std=c++0x
LDFLAGS=/usr/local/lib/libpapi.a

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o perf_test.x xtree_perf.cpp -lpthread $(LDFLAGS) -DUSE_PAPI
	$(CC) $(CFLAGS) -o direct_test.x xtree_direct_perf.cpp -lpthread $(LDFLAGS) -DUSE_PAPI

no_papi:
	$(CC) $(CFLAGS) -o perf_test.x xtree_perf.cpp -lpthread
	$(CC) $(CFLAGS) -o direct_test.x xtree_direct_perf.cpp -lpthread

clean:
	rm *.x

