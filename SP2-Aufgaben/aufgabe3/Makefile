.PHONY: all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700 -Wsign-conversion

all: rush

clean:
	rm -f rush
	rm -f rush.o
	rm -f .h.gch

rush: rush.o plist.o shellutils.o
	$(CC) -o $@ $^

rush.o: rush.c plist.h shellutils.h
	$(CC) $(CFLAGS) -c $<