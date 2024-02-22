.PHONY: all clean

CFLAGS = -std=c11 -pedantic -Wall -Werror -Wextra -Wsign-conversion -D_XOPEN_SOURCE=700
CC = gcc

all: snail

clean:
		rm -f snail
		rm -f snail.o

snail: snail.o
	$(CC) $(CFLAGS) -o snail snail.o

snail.o: snail.c
	$(CC) $(CFLAGS) -c snail.c

