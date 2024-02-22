.PHONY: all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -D_XOPEN_SOURCE=700

all: clash

clean:
	rm -f clash
	rm -f clash.o
	rm -f plist.o


clash: clash.o plist.o
	$(CC) $(CFLAGS) -o clash clash.o plist.o

clash.o: clash.c plist.h
	$(CC) $(CFLAGS) -c clash.c

plist.o: plist.c
	$(CC) $(CFLAGS) -c plist.c

