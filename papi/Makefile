#make file for papi example

CC=g++
CFLAGS= -O2 -Wall
LDFLAGS = /usr/local/lib/libpapi.a

all:
	$(CC) $(CFLAGS) -o hello.x hello_papi.cpp $(LDFLAGS)

clean:
	rm *.x