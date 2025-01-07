.PHONY: all clean

CC=gcc
CFLAGS= -std=c11 -pthread -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700

all: mach

clean:
	rm -f mach
	rm -f queue.o

mach: mach.o sem.o queue.o run.o
	$(CC) -o mach mach.o sem.o queue.o run.o

queue.o: queue.c queue.h sem.h
	$(CC) $(CFLAGS) -c queue.c
