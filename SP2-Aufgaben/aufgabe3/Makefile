.PHONY: all clean

CC = clang
CFLAGS = -std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700 -Wsign-conversion

all:
	rush
##I----> #--------------------------------------------------------------------#
         # #K So wie das hier steht ist 'rush' keine Abhängigkeit, sondern    #
         # ein Befehl, um 'all' zu generieren. Das ist falsch. Beides müsste  #
         # in einer Zeile stehen. (-1)                                        #
         #-------------------------------------------------------------------#/

clean:
	rm -f rush
	rm -f rush.o
	rm -f .h.gch

rush: rush.o plist.o shellutils.o
	$(CC) -o $@ $^

rush.o: rush.c plist.h shellutils.h
	$(CC) $(CFLAGS) -c $<


##P----> #--------------------------------------------------------------------#
         # Punktabzug in dieser Datei: 1.0 Punkte                             #
         #-------------------------------------------------------------------#/
