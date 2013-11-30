#makefile for skiptrie correctness test

CC=g++
CFLAGS= -Wall -g -std=c++0x
LDFLAGS=/usr/local/lib/libpapi.a

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o skiptrie_test.x skiptrie_test.cpp -lpthread $(LDFLAGS) -DUSE_PAPI

no_papi:
	$(CC) $(CFLAGS) -o skiptrie_test.x skiptrie_test.cpp -lpthread

clean:
	rm *.x

