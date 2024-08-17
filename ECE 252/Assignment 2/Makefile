RM = rm -rf

CC = gcc
CFLAGS = -O2 -std=c99 -pthread -D_GNU_SOURCE

all: bin nqueens

bin:
	mkdir -p bin

nqueens: nqueens.c
	$(CC) $< $(CFLAGS) -o bin/$@

clean:
	$(RM) bin

.PHONY: all
