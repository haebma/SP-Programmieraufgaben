#include "halde.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

// TODO:
// malloc head == NULL if-clause



// error values
#define NOVALIDNUMBER 1

/// Magic value for occupied memory chunks.
#define MAGIC ((void*)0xbaadf00d)

/// Size of the heap (in bytes).
#define SIZE (1024*1024*1)

/// Memory-chunk structure.
struct mblock {
	struct mblock *next;
	size_t size;
	char memory[];
};

/// Heap-memory area.
static char *memory;

/// Pointer to the first element of the free-memory list.
static struct mblock *head;

/// Helper function to visualise the current state of the free-memory list.
void printList(void) {
	struct mblock *lauf = head;

	// Empty list
	if (head == NULL) {
		char empty[] = "(empty)\n";
		write(STDERR_FILENO, empty, sizeof(empty));
		return;
	}

	// Print each element in the list
	const char fmt_init[] = "(off: %7zu, size:: %7zu)";
	const char fmt_next[] = " --> (off: %7zu, size:: %7zu)";
	const char * fmt = fmt_init;
	char buffer[sizeof(fmt_next) + 2 * 7];

	while (lauf) {
		size_t n = snprintf(buffer, sizeof(buffer), fmt
			, (uintptr_t) lauf - (uintptr_t)memory, lauf->size);
		if (n) {
			write(STDERR_FILENO, buffer, n);
		}

		lauf = lauf->next;
		fmt = fmt_next;
	}
	write(STDERR_FILENO, "\n", 1);
}

void *malloc (size_t size) {
	if (size == 0){
		errno = NOVALIDNUMBER;
		return NULL;
	}
	
	// Speicher einmalig vom Betriebssystem anfordern
	if (memory == NULL){
		memory = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (memory == MAP_FAILED){
			return NULL;
		}
		
		head = (struct mblock*) memory;
		head->next = NULL;
		head->size = SIZE - sizeof(struct mblock);
	}
	
	// Freispeicherliste nach mblock mit ausreichend Speicher durchsuchen
	struct mblock *lauf = head;
	struct mblock *schlepp = head;

	while (lauf->next != NULL){
		/*if (lauf->next == MAGIC){
			// passiert nie, da Element aus Freispeicherliste entfernt wird
		}*/
		if (lauf->size >= size){ // neuer Speicher kann (mit oder ohne mblock) vergeben werden
			break;
		} else {
			schlepp = lauf;
			lauf = lauf->next;
		}
	}

	// gesamte Freichspeicherliste durchlaufen, aber kein Block mit genug Speicher mehr verfuegbar
	if (lauf->size < size){
		errno = ENOMEM;
		return NULL;
	}
	
	// letztes Stueck Speicher: darf ohne mblock noch vergeben werden
	if (lauf->size <= sizeof(struct mblock) + size){
		if (lauf == head){
			head = lauf->next;
		} else {
			schlepp->next = lauf->next;
		}
		lauf->next = MAGIC;
		return (void *) lauf->memory;
	}

	// size Bytes hinter dem lauf-mblock einen neuen mblock anlegen und initialisieren
	struct mblock* new_mblock = (struct mblock*) (((char*) lauf) + sizeof(struct mblock) + size);
	new_mblock->next = lauf->next;
	new_mblock->size = lauf->size - size - sizeof(struct mblock);

	// head eins weiter verschieben, oder den betreffenden Bereich, falls head->size nicht ausgereicht hat
	if (lauf == head){
		head = new_mblock;
	} else {
		schlepp->next = new_mblock;
	}
	lauf->next = MAGIC;
	lauf->size = size;

	// Pointer auf Bereich nach lauf-mblock returnen
	return (void *) lauf->memory;
}

void free (void *ptr) { 
	if (ptr == NULL){
		return;
	}
	struct mblock* block = (struct mblock*) ptr - 1;
	// falls ptr nicht auf vergebenen Speicher verweist aborten
	if (block->next != MAGIC){
		abort();
	}
	// befreites Element vorne in Liste einfuegen
	block->next = head;
	head = block;
}

void *realloc (void *ptr, size_t size) {
	// Randfaelle
	if (ptr == NULL){
		return malloc(size);
	}
	if (size == 0){
		free(ptr);
		return NULL;
	}
	void * new_pointer = malloc(size);
	if (new_pointer == NULL){
		// errno wird in malloc gesetzt
		return NULL;
	}
	struct mblock *block = (struct mblock*) ptr - 1;
	// kopiere das Minimum der sizes (realloc kann Speicher vergroessern oder verkleinern)
	if (block->size < size){
		memcpy(new_pointer, ptr, block->size);
	} else {
		memcpy(new_pointer, ptr, size);
	}
	
	free(ptr);
	return new_pointer;

}

void *calloc (size_t nmemb, size_t size) {
	// keine Randfaelle, die nicht schon von malloc behandelt werden
	
	void* ptr = malloc(nmemb*size);
	if (ptr == NULL){
		return NULL;
	}
	memset(ptr, 0, nmemb*size);
	return ptr;
}

