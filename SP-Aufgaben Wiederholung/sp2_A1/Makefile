.PHONY: all clean

CC=gcc
CFLAGS= -std=c11 -pedantic -Wall -Werror -Wextra -Wsign-conversion -D_XOPEN_SOURCE=700

all: snail

clean:
	rm -f snail
	rm -f snail.o

snail: snail.o
	$(CC) -o snail snail.o

snail.o: snail.c
	$(CC) $(CFLAGS) -c snail.c