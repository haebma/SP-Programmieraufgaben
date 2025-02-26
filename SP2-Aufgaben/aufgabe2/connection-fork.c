#include "connection.h"
#include "request.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

static void die(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}

int initConnectionHandler(){
    if(initRequestHandler() == -1){
        return -1; // cmdline-arguments are invalid
    }

    // Kinder gehen nicht in Zombie-Zustand über
    struct sigaction act = {
        .sa_handler = SIG_DFL,
        .sa_flags   = SA_NOCLDWAIT,
    };

    if(sigemptyset(&act.sa_mask) == -1) die("sigemptyset");
    if(sigaction(SIGCHLD, &act, NULL) == -1) die("sigaction");

    return 0;
}

void handleConnection(int clientSock, int listenSock){

    pid_t p = fork();
    if(p == -1){
/*I----> +--------------------------------------------------------------------+
         | fork(2) kann auch aufgrund von Ressourcen-Knappheit scheitern, was |
         | im späteren Verlauf wieder gelingen kann. Z.B. kann es sein, dass  |
         | gerade sehr viele Kinder existieren und erst gewartet werden muss  |
         | bis einige von ihnen tot sind. Deshalb soll hier im Fehlerfall das |
         | Program nicht beendet werden. (-1.0)                               |
         +-------------------------------------------------------------------*/
        die("fork");
    }
    else if(p == 0){

        if(close(listenSock) == -1) die("close");

        // Streams für request-http.c öffnen
        FILE *rx = fdopen(clientSock, "r");
        if(rx == NULL) die("fdopen");

        int copy = dup(clientSock);
        if(copy == -1) die("dup");

        FILE *tx = fdopen(copy, "w");
        if(tx == NULL) die("fdopen");

        // Arbeit and request-http.c weitergeben
        handleRequest(rx, tx);

        // sockets + streams schließen, da nicht mehr benötigt
        if(fclose(rx) == EOF) die("fclose");
        if(fclose(tx) == EOF) die("fclose");

        exit(EXIT_SUCCESS);
    } else {
/*I----> +--------------------------------------------------------------------+
         | Hier müsste das clientSocket geschlossen werden. (-0.5)            |
         +-------------------------------------------------------------------*/
        // do nothing
    }
}


/*P----> +--------------------------------------------------------------------+
         | Punktabzug in dieser Datei: 1.5 Punkte                             |
         +-------------------------------------------------------------------*/
