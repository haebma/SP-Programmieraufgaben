#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "plist.h"

#define PATH_MAX 4096
#define LENGTH 1337+1 // 1337 Zeichen inkl. '\n' + '\0'

static void die(char* message){
	perror(message);
	exit(EXIT_FAILURE);
}

static int wordCounter;
static char* cmd;

/*	Arbeitsverzeichnis bekommen
 *
 * cwd in cwdBuffer schreiben
 * falls String zu lang -> puffer solange vergrößern, bis der enorm große Name reinpasst
 * hinten einen Doppelpunkt, gemäß der Aufgabenstellung, anfügen
 *
 * */

static void getAndPrintCWD(){

	char* cwdBuffer = (char *) malloc(PATH_MAX); // hier wird cwd gespeichert
	if(cwdBuffer == NULL){
		die("malloc");
	}

	size_t pathsize = PATH_MAX;

	int i = 1;
	while(getcwd(cwdBuffer, pathsize * i) == NULL){
		if(errno == ERANGE){ // Arbeitsverzeichnis ist zu lang für unseren puffer
			if(i > 10){ // manuell festgelegtes Ende -> cwd zu lang
				fprintf(stderr, "Das Verzeichnis ist zu lang, um zu es speichern.\n");
				exit(EXIT_FAILURE);
			}
			char* newBuffer = realloc(cwdBuffer, PATH_MAX * (++i));
			if(newBuffer == NULL){ // mehr Speicher für cwd-name
				die("realloc");
			}
			errno = 0; // errno zurücksetzen, weil benutzt worden
			cwdBuffer = newBuffer;
		}
		else{
			die("getcwd");
		}
	}
	
	if(fputs(cwdBuffer, stdout) == EOF){ // cwd und : ausgeben
		die("fputs");
	}
	free(cwdBuffer);
	if(fflush(stdout) == EOF){
		die("fflush");
	}
	printf(": ");
}

/*	bekomme argumente und Kommando
 *	
 *	von stdin lesen was user eingibt -> das ist cmdline
 *	argumente werden getrennt und in array (args) zurückgegeben
 *
 */

static char** getArgs(){

	char* cmdline;
	if((cmdline = (char *) malloc(LENGTH)) == NULL){
		die("malloc");
	}

	cmdline = fgets(cmdline, LENGTH, stdin);
	if(cmdline == NULL){
		return NULL;
	}

	// überlange Zeilen werden mit Fehlermeldung verworfen
	char lastChar = cmdline[strlen(cmdline) -1];
	if(lastChar != '\n' && !feof(stdin)){
		while(1){ // Zeile zu Ende lesen
			int next = fgetc(stdin);
			if(next == EOF){
				if(ferror(stdin) != 0){
					die("fgetc");
				} else{
					if(fprintf(stderr, "\nEingabe zu lang - wird ignoriert") < 0) die("fprintf");
					return NULL;
				}
			}
			else if(next == '\n'){
				if(fprintf(stderr, "Eingabe is zu lang - wird ignoriert\n") < 0) die("fprintf");
				return  NULL;
			}
		}
	}

	cmd = strdup(cmdline);
	if(cmd == NULL) die("strdup");
	cmd[strlen(cmd) -1] = '\0'; // newline aus cmd entfernen

	char** args = (char** ) malloc(2 * sizeof(char*)); // Speicher fuer die ersten 2 Argumente reserviert
	if(args == NULL) die("malloc");
	args[0] = strtok(cmdline, " \t\n"); // durch '\n' werden leere Zeilen direkt ignoriert und des macht ja eh keinen unterschied, weil des \n immer des ende einer eingabe ist
	wordCounter = 1;
	while((args[wordCounter] = strtok(NULL, " \t\n")) != NULL){
		char** newArgs = realloc(args, sizeof(char*) * (++wordCounter+1)); // Speicher fuer ein weiteres Argument reservieren
		if(newArgs == NULL){
			die("realloc");
		}
		args = newArgs;
	}
	return args;
}

/* printet alle aktuell laufenden Hintergrundprozesse, die in plist gespeichert sind */

static int jobAusgeben(pid_t pid, const char* cmdLine){
	if(printf("%d %s\n", pid, cmdLine) < 0) return -1;
	else return 0;
}

int main(int argc, char* argv[]){

	while(1){

		/* Zombies einsammeln *VOR DEM PROMPT* */
		while(1){ 
			int exitedChild;
			int status = 0;
			if((exitedChild = waitpid(-1, &status, WNOHANG)) == -1){ // WNOHANG -> wartet nicht bis Kindprozess terminiert
				if(errno == ECHILD){ // keine Zombies gefunden
					errno = 0;
					break;
				}
				else if(exitedChild == -1) die("waitpid");
			}
			if(exitedChild == 0) break; // kein status über Prozess verfügbar

			char cmdLineBuffer[1338];
			if(removeElement(exitedChild, cmdLineBuffer, 1338) == -1){ // Kindprozess auf plist entfernen
				fprintf(stderr, "removeElement: a pair with the given pid does not exit");
				continue;
			}
			if(WIFEXITED(status) != 0){
				printf("Exitstatus [%s] = %d\n", cmdLineBuffer, WEXITSTATUS(status));
			} else {
				printf("Exitstatus [%s] = %s\n", cmdLineBuffer, "child didn't exit properly"); // WIFEXITED == 0 -> kann nicht WEXITSTATUS benutzen 
			}
		}

		getAndPrintCWD();

		char** args = getArgs();
		if(args == NULL){
			if(feof(stdin)){ // STRG + D betätigt
				break;
			}
			else if(errno != 0){ // Fehler 
				die("fgets");
			}
			else{ // überlange Zeile
				continue;
			}
		}

		if(args[0] == NULL || strcmp(args[0], "&")== 0) continue; // leere Zeilen oder Hintergrundprozess ohne Kommando

		if(strcmp(args[0], "cd") == 0){ // cd -> change directory
			if(cmd[strlen(cmd)-1] == '&'){ // letztes Zeichen in cmd ist '"&" -> Hintergrundprozess
				if(strcmp(args[wordCounter-1], "&") == 0){ // letztes Argument ist nur "&" -> letztes Token entfernen
					args[wordCounter-1] = '\0'; 
					wordCounter--; // letztes Token entfernt -> ein Argument weniger
				} else {
					args[wordCounter-1][strlen(args[wordCounter-1])-1] = '\0'; // "&" ist nicht eigenes Token -> entfernen
				}
			}
	
			if(wordCounter != 2){
				fprintf(stderr, "usage: cd <directory>\n");
				continue;
			}
			
			if(chdir(args[1]) == -1){
				perror("chdir");
			}	
			continue;
		}

		if(strcmp(args[0], "jobs") == 0 || strcmp(args[0], "jobs&") == 0){ // jobs -> print working backgrund processes
			if(cmd[strlen(cmd)-1] == '&'){ // letztes Zeichen in cmd ist '"&" -> Hintergrundprozess
				if(strcmp(args[wordCounter-1], "&") == 0){ // letztes Argument ist nur "&" -> letztes Token entfernen
					args[wordCounter-1] = '\0';
				} else {
					args[wordCounter-1][strlen(args[wordCounter-1])-1] = '\0'; // "&" ist nicht eigenes Token -> entfernen
				}
			}
			if(wordCounter != 1){
				fprintf(stderr, "usage: jobs\n");
				continue;
			}
			walkList(jobAusgeben);
			continue;
		}

		pid_t p = fork();
		if(p == -1){ // Fehler
			die("fork");
		} else if(p == 0){ // Kindprozess
			if(cmd[strlen(cmd)-1] == '&'){ // letztes Zeichen in cmd ist '"&" -> Hintergrundprozess
				if(strcmp(args[wordCounter-1], "&") == 0){ // letztes Argument ist nur "&" -> letztes Token entfernen
					args[wordCounter-1] = '\0';
				} else {
					args[wordCounter-1][strlen(args[wordCounter-1])-1] = '\0'; // "&" ist nicht eigenes Token -> entfernen
				}
			}

			execvp(args[0], args);
			die("exec");
			
		} else { // Elterprozess
			if(cmd[strlen(cmd)-1] == '&'){		
				cmd[strlen(cmd)-1] = '\0';
				int errorCheck = insertElement(p, cmd); // Hintergrundprozess in plist einfügen
				if(errorCheck == -1){
					fprintf(stderr, "pair already existst");
					errno = 0;
				}
				if(errorCheck == -2){
					fprintf(stderr, "insufficient memory to complete the operation");
				}
			} else{ // Vordergrundprozess
				int status;
				if(waitpid(p, &status, 0) == -1){ // warten bis Kind terminiert ist
					die("waitpid");
				}
				
				if(WIFEXITED(status) != 0){
					printf("Exitstatus [%s] = %d\n", cmd, WEXITSTATUS(status));
				
				} else {
					printf("Exitstatus [%s] = %s\n", cmd, "child didn't exit properly");
				}
			}
		}	
	}

	exit(EXIT_SUCCESS);
}
