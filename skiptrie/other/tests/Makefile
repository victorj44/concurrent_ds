#makefile for concurrent tests

CC=g++
CFLAGS= -Wall -g -O2
LDFLAGS=

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o sl_test.x sl_test.cpp -lpthread
	$(CC) $(CLFAGS) -o strie_test.x strie_test.cpp -lpthread
	$(CC) $(CFLAGS) -o xtree_test.x xtree_test.cpp -lpthread
	$(CC) $(CFLAGS) -o xtree_hash_test.x xtree_hash_test.cpp -lpthread
#	$(CC) $(CFLAGS) -o dumb_test.x dumb_test.cpp -lpthread	
	$(CC) $(CFLAGS) -o skip_hash_test.x skip_hash_test.cpp -lpthread

clean:
	rm *.x