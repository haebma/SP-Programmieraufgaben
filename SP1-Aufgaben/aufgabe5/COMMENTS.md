

# Zusammenfassung

Sehr gut gemacht, bis auf ein paar kleine Fehler ist alles genau so
wie es sein sollte :)

**Punktzahl:** 13.5 (Maximal: 14, Abzug: 1.5, Bonus: 1)

---


# Dateien


## Datei `Makefile`

Keine Anmerkungen.


## Datei `halde.c`


### `malloc`: `ENOSUCHERRNOCODE` (-0.5)

    68  	if (size == 0){
    69  		errno = NOVALIDNUMBER;
    70  		return NULL;
    71  	}

Aufpassen, man kann nicht &bdquo;belibige&ldquo; `errno` Werte ausdenken.  Ihr
habt ja weiter oben `NOVALIDNUMBER` als 1 definiert gehabt (eigentlich
`EPERM`, &bdquo;Operation not permitted&ldquo;, zumindest *auf Linux-Systemen*),
aber allgemein ist die Bedeutung von der Zahl 1 nicht festgelegt.  Man
sollte sich an die Liste der symbolisch definierten Fehlercodes aus
`errno(3)` halten, da nur die Portabel sind.


### `malloc`: Randfall: `head` ist `NULL` (-1)

    86  	struct mblock *lauf = head;
    87  	struct mblock *schlepp = head;
    88  
    89  	while (lauf->next != NULL){

Sollte die Freispeicherliste initialisiert sein, aber kein Element ist
verfügbar, dann wird `head` auf `NULL` verweisen, und daher auch
`lauf->next` hier ein ungültigen Speicherzugriff verursachen.


## Datei `test.c`

Keine Anmerkungen.

---


# Compiler-Output


## Mit Notwendigen Flags

    halde.c: In function ‘malloc’:
    halde.c:75:89: error: ‘MAP_ANONYMOUS’ undeclared (first use in this function)
       75 |                 memory = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
          |                                                                                         ^~~~~~~~~~~~~
    halde.c:75:89: note: each undeclared identifier is reported only once for each function it appears in
    gmake: *** [Makefile:21: halde.o] Error 1
    gmake: Target 'all' not remade because of errors.


## Mit Zusätzlichen Flags

    In file included from test.c:5:
    halde.h:14:7: warning: redundant redeclaration of ‘malloc’ [-Wredundant-decls]
       14 | void *malloc(size_t size);
          |       ^~~~~~
    In file included from test.c:2:
    /usr/include/stdlib.h:553:14: note: previous declaration of ‘malloc’ with type ‘void *(size_t)’ {aka ‘void *(long unsigned int)’}
      553 | extern void *malloc (size_t __size) __THROW __attribute_malloc__
          |              ^~~~~~
    halde.h:25:6: warning: redundant redeclaration of ‘free’ [-Wredundant-decls]
       25 | void free(void *ptr);
          |      ^~~~
    /usr/include/stdlib.h:568:13: note: previous declaration of ‘free’ with type ‘void(void *)’
      568 | extern void free (void *__ptr) __THROW;
          |             ^~~~
    halde.h:40:7: warning: redundant redeclaration of ‘realloc’ [-Wredundant-decls]
       40 | void *realloc(void *ptr, size_t size);
          |       ^~~~~~~
    /usr/include/stdlib.h:564:14: note: previous declaration of ‘realloc’ with type ‘void *(void *, size_t)’ {aka ‘void *(void *, long unsigned int)’}
      564 | extern void *realloc (void *__ptr, size_t __size)
          |              ^~~~~~~
    halde.h:51:7: warning: redundant redeclaration of ‘calloc’ [-Wredundant-decls]
       51 | void *calloc(size_t nmemb, size_t size);
          |       ^~~~~~
    /usr/include/stdlib.h:556:14: note: previous declaration of ‘calloc’ with type ‘void *(size_t,  size_t)’ {aka ‘void *(long unsigned int,  long unsigned int)’}
      556 | extern void *calloc (size_t __nmemb, size_t __size)
          |              ^~~~~~
    halde.c: In function ‘printList’:
    halde.c:56:25: warning: format not a string literal, argument types not checked [-Wformat-nonliteral]
       56 |                         , (uintptr_t) lauf - (uintptr_t)memory, lauf->size);
          |                         ^
    halde.c:55:28: warning: conversion to ‘size_t’ {aka ‘long unsigned int’} from ‘int’ may change the sign of the result [-Wsign-conversion]
       55 |                 size_t n = snprintf(buffer, sizeof(buffer), fmt
          |                            ^~~~~~~~
    halde.c: In function ‘malloc’:
    halde.c:75:89: error: ‘MAP_ANONYMOUS’ undeclared (first use in this function)
       75 |                 memory = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
          |                                                                                         ^~~~~~~~~~~~~
    halde.c:75:89: note: each undeclared identifier is reported only once for each function it appears in
    gmake: *** [Makefile:21: halde.o] Error 1
    gmake: Target 'all' not remade because of errors.

