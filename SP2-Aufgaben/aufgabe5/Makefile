CC       = cc
CPPFLAGS = -D_XOPEN_SOURCE=700
CFLAGS   = -std=c11 -pedantic -Wall -Werror -pthread -g
LIBS     = -lpthread
RM       = rm -f

# If any of the files in the following list does not exist, the corresponding .o
# module is taken from the libraries libmother.a or libjbuffer.a, respectively.
SOURCES = $(shell ls mother.c connection-mt.c jbuffer.c request-httpx.c sem.c \
                  2> /dev/null)
OBJS    = $(SOURCES:%.c=%.o)

.PHONY: all clean

all: mother

clean:
	$(RM) mother *.o

mother: $(OBJS) libmother.a libjbuffer.a
	$(CC) -o $@ $(LDFLAGS) $(OBJS) -L. -lmother -ljbuffer $(LIBS)

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

mother.o:        mother.c connection.h cmdline.h request.h
connection-mt.o: connection-mt.c connection.h cmdline.h jbuffer.h request.h
request-httpx.o: request-httpx.c request.h cmdline.h dirlisting.h i4httools.h
jbuffer.o:       jbuffer.c jbuffer.h sem.h
sem.o:           sem.c sem.h



##P----> #--------------------------------------------------------------------#
         # Punktabzug in dieser Datei: 0.0 Punkte                             #
         #-------------------------------------------------------------------#/
