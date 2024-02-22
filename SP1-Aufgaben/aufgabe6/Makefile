.PHONY: all clean

CFLAGS =-std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700

all: creeper

clean:
	-rm -f creeper
	-rm -f creeper.o

creeper: creeper.o argumentParser.o
	gcc $(CFLAGS) -o creeper creeper.o argumentParser.o

creeper.o: creeper.c argumentParser.h
	gcc $(CFLAGS) -c creeper.c