.PHONY: all clean

CFLAGS = -std=c11 -pedantic -Wall -Werror -Wextra -D_GNU_SOURCE
CC = gcc

all: test

clean: 
	rm -f test
	rm -f test-ref
	rm -f test.o
	rm -f halde.o

test: test.o halde.o
	$(CC) $(CFLAGS) -o test test.o halde.o
	
test.o: test.c halde.h
	$(CC) $(CFLAGS) -c test.c

halde.o: halde.c halde.h
	$(CC) $(CFLAGS) -c halde.c

test-ref: test.o halde-ref.o
	$(CC) $(CFLAGS) -o test-ref test.o halde-ref.o