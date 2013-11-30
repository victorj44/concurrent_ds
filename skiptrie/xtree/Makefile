CC=g++
CFLAGS= -02 -Wall -gstabs
LDFLAGS=

#OBJ_FILES=LazySkipList/lsk.o

all:
	$(MAKE) -C perf_test
	$(MAKE) -C tests

no_papi:
	$(MAKE) -C perf_test -e no_papi	
	$(MAKE) -C tests -e no_papi

clean:
	rm -r -f *.o *.x */*.x */*.o
