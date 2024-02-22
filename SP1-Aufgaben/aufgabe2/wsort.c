#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LENGTH 102 // max string length = 100 + '/n/0'

// TODO: fgetc 

static void die(char *message){
	perror(message);
	exit(EXIT_FAILURE); // exit free'd
}

/*  -------- COMPARE 2 STRINGS --------
 * a < b -> return -1
 * a = b -> return 0
 * a > b -> return 1
 * */

static int compare(const void *a, const void *b){
	const char **s1 = (const char **) a;
	const char **s2 = (const char **) b;

	return strcmp(*s1, *s2);
}

int main(int argc, char **argv){

	char **words = (char** ) malloc(20 * sizeof(char*)); // array für die Zeilen
	if (words == NULL){
		die("malloc");
	}
	
	char *word_buf = (char *) malloc(LENGTH);
	if (word_buf == NULL){
		die("malloc");
	}

	int wordCounter = 0;

	/* -------- READ LINES FROM STDIN -------- 
 	*
 	*	stop when EOF
 	*	ignore lines with more than 100 characters -> error message
 	*	ignore blank lines
 	*	as always die when errors occur
	*
	* */

	while(1){

		char *check = fgets(word_buf, LENGTH, stdin);
		if (check == NULL){ // auf Fehler prüfen oder nur EOF
			if (ferror(stdin) != 0){
				die("fgets");
			} else {
				break;
			}
		}
		
		if (word_buf[0] == '\n'){ // leere Zeile
			continue;
		}	
		
		char lastChar = word_buf[strlen(word_buf) - 1];

		if (lastChar != '\n' && !feof(stdin)){ // zeile zu lang
			
			while(1){
				char next = fgetc(stdin); // next mallocen??
				if(next == EOF){
					if(ferror(stdin) != 0){
						die("fgetc");
					} else{
						if(fprintf(stderr, "\nWort ist zu lang - wird ignoriert\n") < 0) die("fprintf");
						break;
					}
				}
				else  if(next == '\n'){
					if(fprintf(stderr, "Wort ist zu lang - wird ignoriert\n") < 0) die("fprintf");
					break;
				}
			}
			continue;
		}

		if(wordCounter % 20 == 0 && wordCounter != 0){
			char **wordsNew = realloc(words, (wordCounter + 20) * sizeof(char*)); // alle 20 zeilen mehr speicherplatz allokieren
			if(wordsNew == NULL){
				die("realloc");
			}

			words = wordsNew;
		}

		char* word_to_insert = strdup(word_buf); // allokiert Speicher fuer Wort && dupliziert
		if(word_to_insert == NULL) die("strdup");

		words[wordCounter] = word_to_insert;
		
		wordCounter++;
	} 

	free(word_buf);

	qsort(words, wordCounter, sizeof (char*), compare);
	
	/* WRITE SORTED LINES TO STDOUT
	 *
	 * instantly free words that won't be used again
	 *
	 * */

	for(int i = 0; i < wordCounter; i++){
		if(fputs(words[i], stdout) < 0) die("fputs");
		free(words[i]);
	}

	free(words);
	if(fflush(stdout) == EOF) die("fflush");
	exit(EXIT_SUCCESS);
}
