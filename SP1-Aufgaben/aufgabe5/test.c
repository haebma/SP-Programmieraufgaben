#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "halde.h"

int main() {
	printList();

	char *m1 = (char*) malloc(200*1024);
	printList();

	free(m1);
	printList();


	char *m2 = (char*) malloc(10*1024);
	printList();

	int *m3 = (int*) malloc(50*1024);
	printList();

	char *m4 = (char*) malloc(100*1024);
	printList();

	free(m3);
	printList();

	char *m5 = (char*) malloc(10*1024);
	printList();

	free(m2);
	printList();

	free(m5);
	printList();

	free(m4);
	printList();

// Randfall: malloc(0) sollte NULL zurueckgeben oder einen Pointer den man free uebergeben kann
	char *m6 = (char*) malloc(0);
	printList();

	free(m6);
	printList();
	
	char *m7 = (char*) malloc(20*1024);
	printList();

	char *m8 = (char*) malloc(10*1024);
	printList();

	char *m9 = (char*) malloc(30*1024);
	printList();

	free(m8);
	printList();

/*// Randfall: zweimal free auf selbem Pointer
	free(m8);
	printList();
*/
	free(m9);
	printList();

	free(m7);
	printList();

	exit(EXIT_SUCCESS);
}