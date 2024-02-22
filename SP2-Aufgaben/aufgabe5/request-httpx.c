#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#include "request.h"
#include "cmdline.h"
#include "i4httools.h"
#include "dirlisting.h"

static void die(char* message){
    perror(message);
    return;
}

// alle Einträge, welche mit '.' beginnen, werden rausgefiltert (return 1);
static int filter(const struct dirent* entry){
    if(entry->d_name[0] == '.') return 0;
    return 1;
}

/* alle Namen der Verzeichniseinträge, die nicht mit Punkt beginnnen, in alphabetischer Reihenfolge ausgeben */
static int printDir(FILE *rx, FILE *tx, const char *abspath, const char *relpath){

    struct dirent **namelist;
    int n;

    n = scandir(abspath, &namelist, filter, alphasort);
    if(n == -1){
        perror("scandir");
        return -1;
    }

    httpOK(tx);

    // namelist ausgeben mithilfe von "dirlisting.h"
    dirlistingBegin(tx, relpath);

    for(int i = 0; i < n; i++){
        dirlistingPrintEntry(tx, relpath, namelist[i]->d_name);
        free(namelist[i]);
    }

    dirlistingEnd(tx);

    free(namelist);
    return 0;
}

static int executePerl(FILE *rx, FILE *tx, const char* abspath, const char* relpath){

    pid_t pid = fork();
    if(pid == -1){                                  // error
        httpInternalServerError(tx, relpath);
        return -1;
    }
    if(pid == 0){                                   // child-process
        // redirect stdout
        int fdTX;                                   // file-descriptor für tx
        if((fdTX = fileno(tx)) == -1){
            httpInternalServerError(tx, relpath);
            return -1;
        }

        if(dup2(fdTX, STDOUT_FILENO) == -1){
            httpInternalServerError(tx, relpath);
            return -1;
        }

        execl(abspath, abspath, (char*) NULL);
/*I----> +--------------------------------------------------------------------+
         | Nach dem exec() muss ein exit() stehen! Es muss sichergestellt     |
         | sein, dass sich der Kindprozess beendet. (-0.5)                    |
         +-------------------------------------------------------------------*/
        return -1;
    }
    else{                                           // parent-process
        // nothing to do
    }

    return 0;
}

static int printFile(FILE* tx, FILE *fp, const char* abspath, const char* relpath){

    // pathname is file
    if (fp == NULL){ // several causes of the error
        if (errno == EACCES || errno == EFAULT){
            httpForbidden(tx, abspath);
        } else if (errno == ENOENT || errno == ENOTDIR){
            httpNotFound(tx, abspath);
        } else if (errno == ENAMETOOLONG || errno == ELOOP){
            httpBadRequest(tx);
        } else {
            httpInternalServerError(tx, abspath);
            perror("fopen");
        }
        return -1;
    }

    // found file
    httpOK(tx);

    // DATA
    int c;
    while(1){
        // get Inhalt
        c = fgetc(fp);
        if (c == EOF){
            break;
        }
        if (fputc(c, tx) == EOF){
            fprintf(stderr, "Error in fputc\n");
            return -1;
        }
        if (c == '\n'){ // flush on every new line
            if (fflush(tx)){
                perror("fflush");
                return -1;
            }
        }
    }

    if(!feof(fp)){ // c = EOF because of error in fgetc
        fprintf(stderr, "Error in fgetc\n");
        return -1;
    }

    if (fflush(tx)){ // flush last time
        perror("fflush");
        return -1;
    }

    // close filedescriptor; don't send error on tx, because document is sent fully, client doesn't care, only server is affected
    if (fclose(fp)){
        perror("fclose");
        return -1;
    }

    return 0;
}

int initRequestHandler(){

    struct sigaction act = {                        // Zombies sofort aufsammeln
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT,
    };

    if(sigemptyset(&act.sa_mask) == -1) die("sigemptyset");
    if(sigaction(SIGCHLD, &act, NULL) == -1) die("sigaction");

    const char *path = cmdlineGetValueForKey("wwwpath");
    if(path == NULL) return -1;

    if(strlen(path) == 0) return -1;

    return 0;
}

void handleRequest(FILE *rx, FILE *tx){


    char *request = (char *) malloc(8192 + 2 + 1);  // max 8192 chars + \r\n + \0
    if (request == NULL){
        httpInternalServerError(tx, NULL);          // error in malloc
        return;
    }

    request = fgets(request, 8194,  rx);
    if (request == NULL){
        httpInternalServerError(tx, NULL);
/*I      +-A-----------------------------------------------------------------+
         | An sich ist das auch ein Fall von BadRequest...                    |
         +-------------------------------------------------------------------*/
        fprintf(stderr, "Error or EOF in fgets\n");
        free(request);
        return;
    }

    char *saveptr;
    const char *get = strtok_r(request, " ", &saveptr);
    if (get == NULL){
        httpBadRequest(tx);
        free(request);
        return;
    }

    const char *relpath = strtok_r(NULL, " ", &saveptr);            // relative path
    if (relpath == NULL) {
        httpBadRequest(tx);
        free(request);
        return;
    }

    const char *http = strtok_r(NULL, "\n\r", &saveptr);            // put \r\n NOT into the buffer
    if (http == NULL){
        httpBadRequest(tx);
        free(request);
        return;
    }

    if (strcmp(get, "GET") != 0 || (strcmp(http, "HTTP/1.0") != 0 && strcmp(http, "HTTP/1.1") != 0)){
        // request doesn't start with 'GET' or doesn't end with 'HTTP/1.0' / 'HTTP/1.1'
        httpBadRequest(tx);
        free(request);
        return;
    }

    if (checkPath(relpath) == -1){
        // path ascends beyond the web directory
        httpForbidden(tx, relpath);
        free(request);
        return;
    }

    // merge complete path
    const char *wwwpath = cmdlineGetValueForKey("wwwpath");
    int laenge = strlen(wwwpath) + strlen(relpath) + 1;
    char abspath[laenge];                           // absolute path

    strcpy(abspath, wwwpath);                       // copy, weil strcat nimmt keine const char
    strcat(abspath, relpath);

    // search for path on server; in the www-path there should only be files that are allowed to read -> no error handling for no rights needed
    errno = 0;

    struct stat statbuf;
    if (stat(abspath, &statbuf) == -1){
        perror("stat");
        free(request);
        httpNotFound(tx, abspath);
        return;
    }

    // pathname is directory
    if (S_ISDIR(statbuf.st_mode)){
        if (strlen(abspath) >= 1 && abspath[strlen(abspath)-1] != '/'){     // endet nicht auf '/'
            // httpMovedPermanently mit Hinweis auf neuen aktuellen Pfad
            char completePath[strlen(relpath) + 2];
            strcpy(completePath, relpath);
            strcat(completePath, "/");
            httpMovedPermanently(tx, completePath);
            free(request);
            return;
        } else {
            char index_path[strlen(abspath) + strlen("index.html") + 1];
            strcpy(index_path, abspath);
            strcat(index_path, "index.html");

            // check if index.html exists in directory
            errno = 0;
            FILE *fp = fopen(index_path, "r");

            if(errno == ENOENT){      // file doesn't exists
                printDir(rx, tx, abspath, relpath);
                free(request);
                return;
            } else {                                // file exist
                // check if index_path is regular file
                printFile(tx, fp, index_path, relpath);
                free(request);
                return;
            }
        }
    }

    // check, ob es eine reguläre Datei ist && Ausführrechte
    if(S_ISREG(statbuf.st_mode)){
        if (strlen(abspath) >= 4   // check auf perl-Skript
        && abspath[strlen(abspath) - 3] == '.'
        && abspath[strlen(abspath) - 2] == 'p'
        && abspath[strlen(abspath) - 1] == 'l'){
            // check if executable
            if(statbuf.st_mode & S_IXUSR){
                executePerl(rx, tx, abspath, relpath);
            }
            free(request);
            return;
        }
        else{
            FILE *fp = fopen(abspath, "r");
            printFile(tx, fp, abspath, relpath);
            free(request);
            return;
        }
    } else {        // keine reguläre Datei oder Directory oder Perl Skript
        httpNotFound(tx, abspath);
        free(request);
        return;
    }
}


/*P----> +--------------------------------------------------------------------+
         | Punktabzug in dieser Datei: 0.5 Punkte                             |
         +-------------------------------------------------------------------*/
