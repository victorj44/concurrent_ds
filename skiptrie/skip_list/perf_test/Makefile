#makefile for concurrent tests

CC=g++
CFLAGS= -Wall -g -O2 -std=c++0x
LDFLAGS=/usr/local/lib/libpapi.a

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o perf_test.x skip_list_perf.cpp -lpthread $(LDFLAGS) -DUSE_PAPI

no_papi:
	$(CC) $(CFLAGS) -o perf_test.x skip_list_perf.cpp -lpthread

clean:
	rm *.x
