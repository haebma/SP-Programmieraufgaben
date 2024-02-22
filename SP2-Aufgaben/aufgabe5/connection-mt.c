#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>

#include "connection.h"
#include "request.h"
#include "cmdline.h"
#include "jbuffer.h"

static BNDBUF *buffer;              // Buffer für Socket-Deskriptoren

static void die(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}

static long parse_positive_long_or_minus_one(const char *str){
        errno = 0;
        char *endptr;
        long x = strtol(str, &endptr, 10);

        if(errno) return -1;

        // Non empty string was parsed
        if(str == endptr || *endptr != '\0') {
            return -1;
        }
        if(x <= 0){
            return -1;
        }
        return x;
}

static void *execute(){

    for(;;){
        int clientSock = bbGet(buffer);

        int flags = fcntl(clientSock, F_GETFD, 0);
        if(flags == -1){
            perror("fcntl");
            continue;
        }
        if(fcntl(clientSock, F_SETFD, flags | FD_CLOEXEC) == -1){
            perror("fcntl");
            continue;
        }

        FILE *rx = fdopen(clientSock, "r");
        if(rx == NULL){
/*I----> +--------------------------------------------------------------------+
         | `close(clientSock)` fehlt. (-1)                                    |
         +-------------------------------------------------------------------*/
            perror("fdopen");
            continue;
        }

        int copy = dup(clientSock);
        if(copy == -1){
/*I----> +--------------------------------------------------------------------+
         | `fclose(rx)` fehlt. (-1)                                           |
         +-------------------------------------------------------------------*/
            perror("dup");
            continue;
        }

        if(fcntl(copy, F_SETFD, flags | FD_CLOEXEC) == -1) {
/*I----> +--------------------------------------------------------------------+
         | `fclose(rx)`/`close(copy)` fehlt.                                  |
         +-------------------------------------------------------------------*/
            perror("fcntl");
            continue;
        }

        FILE *tx = fdopen(copy, "w");
        if(tx == NULL){
/*I----> +--------------------------------------------------------------------+
         | `fclose(rx)`/`fclose(tx)` fehlt.                                   |
         +-------------------------------------------------------------------*/
            perror("fdopen");
            continue;
        }

        handleRequest(rx, tx);

        if(fclose(rx) == EOF){
            perror("fclose");
            continue;
        }
        if(fclose(tx) == EOF){
            perror("fclose");
            continue;
        }
    }

    return NULL;
}

// Thread-Pool erstellen
// Bounded Buffer erstellen
// Signalbehandlung erstellen

int initConnectionHandler(){
    if(initRequestHandler() == -1){
        return -1;
    }

    // SIGPIPE ignorieren
    struct sigaction act = {
        .sa_handler = SIG_IGN,
    };

    if(sigaction(SIGPIPE, &act, NULL) == -1) die("sigaction");

    long threads;
    size_t bufsize; // size_t is genauso breit wie ein long

    // anzahl threads von cmdline auslesen oder standardmäßig als 4 initialisieren
    const char* strthreads = cmdlineGetValueForKey("threads");
    if(strthreads == NULL) threads = 4;
    else threads = parse_positive_long_or_minus_one(strthreads);
    if(threads < 0) return -1;

    // Buffergröße von cmdline auslesen oder standardmäßig als 8 initialisieren
    const char* strbufsize = cmdlineGetValueForKey("bufsize");
    if(strbufsize == NULL) bufsize = 8;
    else bufsize = parse_positive_long_or_minus_one(strbufsize);
    if(bufsize < 0) return -1;

    // Buffer erstellen
    buffer = bbCreate(bufsize);
    if(buffer == NULL) {
        fprintf(stderr, "Error in bbCreate\n");
        exit(EXIT_FAILURE);
    }

    // Thread-Pool erstellen
    pthread_t tid[threads];
    for(int i = 0; i < threads; i++){
        errno = pthread_create(&tid[i], NULL, execute, NULL);
        if(errno){
            die("pthread_create");
        }
    }

    return 0;
}

// socketDeskriptor in buffer einfügen
void handleConnection(int clientSock, int listenSock){
/*I----> +--------------------------------------------------------------------+
         | Es ist zwingend notwendig das close-on-exec-flag auf clientSock    |
         | bereits hier VOR dem Hinzufügen zum Puffer vorzunehmen. Sonst ist  |
         | es bei einem sehr großen Puffer, schnell hintereinander            |
         | eintreffender Anfragen und wenig Threads mit steigender            |
         | Wahrscheinlichkeit möglich, dass ein anderer Thread ein exec       |
         | ausfuehrt bevor dieser Dateideskriptor mit dem flag versehen wird. |
         | Dadurch dass sich alle Threads einen Adressraum teilen können auch |
         | über Threads hinweg die Dateideskriptoren an Kinder vererbt        |
         | werden. Das ist ein Problem das sich nicht vollends lösen laesst,  |
         | aber den Dateideskriptor hier u.U. lange ungeschützt in der Queue  |
         | zu lassen ist keine gute Idee. Mehr dazu in der T.UE. :) (-0.5)    |
         +-------------------------------------------------------------------*/

    bbPut(buffer, clientSock);

}


/*P----> +--------------------------------------------------------------------+
         | Punktabzug in dieser Datei: 2.5 Punkte                             |
         +-------------------------------------------------------------------*/
