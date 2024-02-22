.PHONY: all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700 -Wsign-conversion

all:
	sister

clean:
	rm -f sister
	rm -f sister.o
	rm -f connection-fork.o
	rm -f request-http.o
	rm -f *.h.gch

sister: sister.o connection-fork.o request-http.o cmdline.o i4httools.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

sister.o: sister.c cmdline.h connection.h

request-http.o: request-http.c request.h i4httools.h cmdline.h

connection-fork.o: connection-fork.c connection.h request.h


##P----> #--------------------------------------------------------------------#
         # Punktabzug in dieser Datei: 0.0 Punkte                             #
         #-------------------------------------------------------------------#/
