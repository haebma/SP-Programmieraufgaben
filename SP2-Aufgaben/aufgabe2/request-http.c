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
        return -1;
    }

    return 0;
}


void handleRequest(FILE *rx, FILE* tx){

    char *request = (char *) malloc(8192 + 2 + 1); // max 8192 chars + \r\n + \0
    if (request == NULL){
        httpInternalServerError(tx, NULL); // error in malloc
        die("malloc");
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
    strcpy(path, wwwpath); // copy, weil strcat nimmt keine const char
    strcat(path, newpath);

    // search for path on server; in the www-path there should only be files that are allowed to read -> no error handling for no rights needed
    errno = 0;
    FILE *fp = fopen(path, "r");
    if (fp == NULL){ // several causes of the error
        if (errno == EACCES || errno == EFAULT){
            httpForbidden(tx, path);
        } else if (errno == ENOENT || errno == ENOTDIR){
            httpNotFound(tx, path);
        } else if (errno == ENAMETOOLONG || errno == ELOOP){
            httpBadRequest(tx);
        } else if (errno == ENOTDIR || errno == ENOENT){
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
            fprintf(stderr, "Error in fputc\n");
            exit(EXIT_FAILURE);
        }
        if (c == '\n'){ // flush on every new line
            if (fflush(tx)){
                httpInternalServerError(tx, path);
                die("fflush");
            }
        }
    }
   
    if(!feof(fp)){ // c = EOF because of error in fgetc
        httpInternalServerError(tx, path);
        fprintf(stderr, "Error in fgetc\n");
        exit(EXIT_FAILURE);
    }

    if (fflush(tx)){ // flush last time
        httpInternalServerError(tx, path);
        die("fflush");
    }

    // close filedescriptor; don't send error on tx, because document is sent fully, client doesn't care, only server is affected
    if (fclose(fp)){
        die("fclose");
    }

    exit(EXIT_SUCCESS);

    // callers responsibility
    // fclose(rx);
    // fclose(tx);
}
