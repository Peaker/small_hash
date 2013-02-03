CC=gcc ${CFLAGS}
CFLAGS=-O2 -Wall -g -c
LD=gcc

all: example benchmark

%.o: %.c Makefile
	${CC} -o $@ $<

benchmark.o example.o small_hash.o: small_hash.h small_hash_internals.h

EXAMPLE_OBJS=example.o small_hash.o
example: ${EXAMPLE_OBJS} Makefile
	${LD} -o $@ ${EXAMPLE_OBJS}

BENCHMARK_OBJS=benchmark.o small_hash.o
benchmark: ${BENCHMARK_OBJS} Makefile
	${LD} -o $@ ${BENCHMARK_OBJS}

clean:
	-rm example example.o benchmark benchmark.o small_hash.o
