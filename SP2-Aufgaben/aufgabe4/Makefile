.PHONY: all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -pthread -Wall -Werror -Wextra -DXOPEN_SOURCE=700 -Wsign-conversion

all: jbuffer-test jbuffer-test-dynamic

clean:
	rm -f jbuffer-test
	rm -f jbuffer-test-dynamic
	rm -f libjbuffer.a
	rm -f libjbuffer-dynamic.so
	rm -f *.o
	rm -f *.gch

# c) Statische Bibliothek
jbuffer-test: jbuffer-test.o libjbuffer.a
	$(CC) -pthread -o $@ $< -L//home/cip/2021/he35nyky/sp2/aufgabe4 -ljbuffer

jbuffer-test.o: jbuffer-test.c jbuffer.h
	$(CC) $(CFLAGS) -c -o $@ $<

libjbuffer.a: jbuffer.o sem.o
	ar -rcs $@ $^

jbuffer.o: jbuffer.c sem.h
	$(CC) $(CFLAGS) -c -o $@ $<

sem.o: sem.c
	$(CC) $(CFLAGS) -c -o $@ $<


# d) Dynamische Bibliothek
jbuffer-test-dynamic: jbuffer-test.o libjbuffer-dynamic.so
	$(CC) -o $@ $< -L//home/cip/2021/he35nyky/sp2/aufgabe4 -ljbuffer-dynamic

jbuffer-test-dynamic.o: jbuffer-test.c jbuffer.h
	$(CC) $(CFLAGS) -c -o $@ $<

libjbuffer-dynamic.so: jbuffer-dynamic.o sem-dynamic.o
	$(CC) -shared $(CFLAGS) -o $@ $^

jbuffer-dynamic.o: jbuffer.c sem.h jbuffer.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

sem-dynamic.o: sem.c sem.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<