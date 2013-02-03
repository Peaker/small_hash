CC=gcc ${CFLAGS}
CFLAGS=-O0 -Wall -g -c
LD=gcc

all: example

%.o: %.c
	${CC} -o $@ $<

example.o small_hash.o: small_hash.h small_hash_internals.h

example: example.o small_hash.o
	${LD} -o $@ $^

clean:
	-rm example example.o small_hash.o
