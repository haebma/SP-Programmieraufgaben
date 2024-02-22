#include "i4httools.h"
#include "cmdline.h"
#include "request.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>

static void die(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

// return -1 if cmdline-args invalid; die on non-recoverable errors like allocation failure
int initRequestHandler(){
    // path-flag set?
    const char *path = cmdlineGetValueForKey("wwwpath");
    if (path == NULL){
        return -1;
    }

    // path is too long
    if (strlen(path) > 8192 - strlen("GET ") - strlen("HTTP/1.0")){ // \r\n is ignored (8192 +\r\n\0)
/*I----> +--------------------------------------------------------------------+
         | Die 8192 Zeichen Limitierung bezieht sich auf die Anfrage, nicht   |
         | auf den wwwpath. Stattdessen sollte allerdings geprüft werden,     |
         | dass der Pfad nicht leer ist (strlen != 0) (-0.5)                  |
         +-------------------------------------------------------------------*/
        return -1;
    }

    return 0;
}


void handleRequest(FILE *rx, FILE* tx){
/*I----> +--------------------------------------------------------------------+
         | Hier wäre es schön zu überprüfen, ob das Modul bereits             |
         | initialisiert wurde.                                               |
         +-------------------------------------------------------------------*/

    char *request = (char *) malloc(8192 + 2 + 1); // max 8192 chars + \r\n + \0
    if (request == NULL){
        httpInternalServerError(tx, NULL); // error in malloc
        die("malloc");
/*I----> +--------------------------------------------------------------------+
         | In der request-http.c sollte kein exit(3p) verwendet werden.       |
         | (siehe API-Doku + "kein exit bei selbst geschriebenen              |
         | Bibliotheksfunktionen") (-0.5)                                     |
         +-------------------------------------------------------------------*/
    }

    request = fgets(request, 8194,  rx);
    if (request == NULL){
        httpInternalServerError(tx, NULL);
        fprintf(stderr, "Error or EOF in fgets\n");
        exit(EXIT_FAILURE);
    }

    const char *get = strtok(request, " ");
    if (get == NULL){
        httpBadRequest(tx);
        exit(EXIT_FAILURE);
    }

    const char *newpath = strtok(NULL, " ");
    if (newpath == NULL) {
        httpBadRequest(tx);
        exit(EXIT_FAILURE);
    }

    const char *http = strtok(NULL, "\n\r"); // put \r\n NOT into the buffer
    if (http == NULL){
        httpBadRequest(tx);
        exit(EXIT_FAILURE);
    }

    if (strcmp(get, "GET") != 0 || (strcmp(http, "HTTP/1.0") != 0 && strcmp(http, "HTTP/1.1") != 0)){
        // request doesn't start with 'GET' or doesn't end with 'HTTP/1.0' / 'HTTP/1.1'
        httpBadRequest(tx);
        exit(EXIT_FAILURE);
    }

    if (checkPath(newpath) == -1){
        // path ascends beyond the web directory
        httpForbidden(tx, newpath);
        exit(EXIT_FAILURE);
    }

    // merge complete path
    const char *wwwpath = cmdlineGetValueForKey("wwwpath");
    char path[8195];
/*I----> +--------------------------------------------------------------------+
         | Da alleine die Anfrage 8192 Bytes lang sein kann is dieser Puffer  |
         | definitiv zu klein. Stattdessen sollte hier mit `strlen(wwwpath) + |
         | strlen(newpath) + 1` die Länge am besten dynamisch ermittelt       |
         | werden. (-1)                                                       |
         +-------------------------------------------------------------------*/
    strcpy(path, wwwpath); // copy, weil strcat nimmt keine const char
    strcat(path, newpath);

    // search for path on server; in the www-path there should only be files that are allowed to read -> no error handling for no rights needed
    errno = 0;
/*I----> +--------------------------------------------------------------------+
         | Zunächst sollte mit stat(3p) ermittelt werden, ob es sich um eine  |
         | reguläre Datei handelt. (-1)                                       |
         +-------------------------------------------------------------------*/
    FILE *fp = fopen(path, "r");
    if (fp == NULL){ // several causes of the error
        if (errno == EACCES || errno == EFAULT){
            httpForbidden(tx, path);
        } else if (errno == ENOENT || errno == ENOTDIR){
            httpNotFound(tx, path);
        } else if (errno == ENAMETOOLONG || errno == ELOOP){
            httpBadRequest(tx);
        } else if (errno == ENOTDIR || errno == ENOENT){
/*I      +--------A----------------------------------------------------------+
         | Die Bedingung hatten wir oben schonmal, ist also ein wenig         |
         | überflüssig.                                                       |
         +-------------------------------------------------------------------*/
            httpNotFound(tx, path);
            exit(EXIT_FAILURE);
        } else {
            httpInternalServerError(tx, path);
            perror("fopen");
        }
        exit(EXIT_FAILURE);
    }
    free(request);

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
            httpInternalServerError(tx, path);
/*I      +-------A-----------------------------------------------------------+
         | Eine Fehlermeldung zu senden ergibt hier recht wenig Sinn, wenn    |
         | der Socket bereits kaputt ist.                                     |
         +-------------------------------------------------------------------*/
            fprintf(stderr, "Error in fputc\n");
            exit(EXIT_FAILURE);
        }
        if (c == '\n'){ // flush on every new line
            if (fflush(tx)){
/*I      +------A------------------------------------------------------------+
         | Ist an der Stelle nicht direkt falsch, aber ein einzelnes          |
         | fflush(3p) ganz am Ende würde in diesem Fall auch reichen.         |
         +-------------------------------------------------------------------*/
                httpInternalServerError(tx, path);
/*I      +-------A-----------------------------------------------------------+
         | Eine Fehlermeldung zu senden ergibt hier recht wenig Sinn, wenn    |
         | der Socket bereits kaputt ist.                                     |
         +-------------------------------------------------------------------*/
                die("fflush");
            }
        }
    }

    if(!feof(fp)){ // c = EOF because of error in fgetc
        httpInternalServerError(tx, path);
/*I      +-------A-----------------------------------------------------------+
         | Ein "200 OK" wurde bereits an den Client geschickt. Eine zweite    |
         | Nachricht ist deswegen nicht angebracht.                           |
         +-------------------------------------------------------------------*/
        fprintf(stderr, "Error in fgetc\n");
        exit(EXIT_FAILURE);
    }

    if (fflush(tx)){ // flush last time
        httpInternalServerError(tx, path);
/*I      +-------A-----------------------------------------------------------+
         | Ein "200 OK" wurde bereits an den Client geschickt. Eine zweite    |
         | Nachricht ist deswegen nicht angebracht.                           |
         +-------------------------------------------------------------------*/
        die("fflush");
    }

    // close filedescriptor; don't send error on tx, because document is sent fully, client doesn't care, only server is affected
    if (fclose(fp)){
        die("fclose");
    }

    exit(EXIT_SUCCESS);
/*I----> +--------------------------------------------------------------------+
         | In dieser Funktion sollte kein exit(3p) aufgerufen werden. So wie  |
         | hier implementiert kehrt sie nie zurück. Dementsprechend können    |
         | auch die FILE* nicht ordnungsgemäß geschlossen werden.             |
         +-------------------------------------------------------------------*/

    // callers responsibility
    // fclose(rx);
    // fclose(tx);
}


/*P----> +--------------------------------------------------------------------+
         | Punktabzug in dieser Datei: 3.0 Punkte                             |
         +-------------------------------------------------------------------*/
