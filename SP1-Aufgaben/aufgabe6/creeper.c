#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/sysmacros.h>
#include <string.h>
#include <libgen.h>
#include <fnmatch.h>
#include <stdbool.h>

#include "argumentParser.h"

/* TODO: 
* wenn keine maxdepth angegeben ist -> bis zu den blaettern
* kompletten relativen pfad ausgeben
*/

static void die(char* message){
    perror(message);
    exit(EXIT_FAILURE);
}

static int creeper(char* path, long maxdepth, char* name, char* type){

    bool print = true;

    if(maxdepth == 0){ // basisfall fuer rekursion
        return 0;
    }

    DIR *dir = opendir(path);
    if(dir == NULL){
        die("opendir");
    }

    struct dirent *ent;
    while(errno = 0, (ent = readdir(dir)) != NULL){
        char* d_name = ent->d_name;

        struct stat buf;
	if(name != NULL){ // check, ob das pattern passt
		int check = fnmatch(name, d_name, FNM_PERIOD | FNM_PATHNAME); // FNM_PERIOD für Punkte vor Dateinamen und FNM_PATHNAME für Slash muss als Slash dargestellt sein
		if(check == 0) print = true;
		else if(check == FNM_NOMATCH) print = false; // passt nicht
		else if(check != 0) die("fnmatch"); // Fehler bei fnmatch
	}
        if(-1 == lstat(d_name, &buf)){ // informationen ueber aktuelle datei
            //perror("lstat");
            //continue;
        }
        if(S_ISLNK(buf.st_mode)){ // nicht den symbolic links folgen
	    if(print == true){
	    	printf("%s\n", d_name);
	    }
            continue;
    	}
        if(S_ISDIR(buf.st_mode)){ // die directories rekursiv durchlaufen
            if(strcmp(d_name, ".") == 0){
                continue;
            }
            if(strcmp(d_name, "..") == 0){
                continue;
            }
	    if(strcmp(type, "f") != 0 && print == true){ // check, ob nur files ausgegeben werden
	            printf("%s/%s (directory)\n", path, d_name);
	    }
            //TODO: richtigen pfad übergeben, sonst verlieren wir dirname
            creeper(d_name, maxdepth-1, name, type);
        }
        else if(S_ISREG(buf.st_mode)){ // regulaere dateien ausgeben
		if(strcmp(type, "d")!= 0 && print == true){ // check, ob nur directories ausgegeben werden
		     printf("%s/%s\n", path, d_name); 
		}
       }
    }

    if(name == NULL){}

    if(errno) die("readdir");

    if(closedir(dir) == -1) die("closedir");
    return 0;
}

int main(int argc, char* argv[]){

    if(initArgumentParser(argc, argv) < 0){
        fprintf(stderr, "Usage: ./creeper-ref path... [-maxdepth=n] [-name=pattern] [-type={d,f}]\n");
        exit(EXIT_FAILURE);
    }

    char* maxdepth = getValueForOption("maxdepth");
    char* name = getValueForOption("name");
    char* type = getValueForOption("type");

    errno = 0;
    long rektiefe;
    if(maxdepth != NULL){
	    rektiefe = strtol(maxdepth, NULL, 10); // rekursionstiefe in long parsen
	    if(rektiefe == 0 && errno) die("strtol");
    } else {
	    rektiefe = -1; // bis ans Ende jedes Pfads laufen
    }
    if(type == NULL){
    	type = "everything"; // alles soll ausgegeben werden
    }
    if(name == NULL){
    	name = "everything"; // alles soll ausgegeben werden
    }

    int argsCounter = getNumberOfArguments(); // Anzahl der pfade
    char** path = (char**) malloc(argsCounter * 1000);
    if(path == NULL) die("malloc");

    for(int i = 0; i < argsCounter; i++){
        path[i] = getArgument(i);
        if(path[i] == NULL) continue;
	if(strcmp(type, "f") != 0){ // check, ob der path geprintet werden darf
		if(name == NULL) printf("%s\n", path[i]);
		else{
			int check = fnmatch(name, basename(path[i]), FNM_PERIOD | FNM_PATHNAME);
			if(check == 0) printf("%s\n", path[i]);
			else if(check != FNM_NOMATCH && check != 0) die("fnmatch");
		}
	}
       	creeper(path[i], rektiefe, name, type);
    }

    free(path);

    if(fflush(stdout) == EOF) die("fflush");

    exit(EXIT_SUCCESS);
}
