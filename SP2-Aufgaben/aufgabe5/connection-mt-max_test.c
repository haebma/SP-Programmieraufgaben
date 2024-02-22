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

static BNDBUF *buffer;

static int parse_positive_long_or_die(const char *str){
        errno = 0;
        char *endptr;
        long x = strtol(str, &endptr, 10);
        //if(errno) die("invalid number");
        if (errno) return -1;
        // Non empty string was parsed
        if(str == endptr || *endptr != '\0') {
            // fprintf(stderr, "invalid number\n");
            // exit(EXIT_FAILURE);
            return -1;
        }
        if(x <= 0){
            // fprintf(stderr, "number not positive\n");
            // exit(EXIT_FAILURE);
            return -1;
        }
        return (int)x;    
}

static void *startRoutine(){
    // take sockets out of jbuffer and invoke handleConnection?
    for(;;){
        int clientSock = bbGet(buffer);
        // Rest siehe connection-mt.c


    }
    return NULL;
}

int initConnectionHandler(void){
    if (initRequestHandler()) return -1;

    // ignore SIGPIPE
    struct sigaction act = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT,
    };

    if (sigemptyset(&act.sa_mask) == -1) die("sigemptyset");
    if (sigaction(SIGPIPE, &act, NULL) == -1) die ("sigaction");

    const char *path = cmdlineGetValueForKey("wwwpath");
    if (path == NULL) return -1;

    //const char *c_port = cmdlineGetValueForKey("port");
    const char *c_bufsize = cmdlineGetValueForKey("bufsize");
    const char *c_threads = cmdlineGetValueForKey("threads");
    size_t bufsize, threads;
    //if (c_port == NULL) port = 
    if (c_bufsize == NULL) bufsize = 8;
    else {
        bufsize = parse_positive_long_or_die(c_bufsize);
        if (bufsize < 0) return -1;
    }
    if (c_threads == NULL) threads = 4;
    else {
        threads = parse_positive_long_or_die(c_threads);
        if (threads < 0) return -1;
    }
    // create jbuffer
    buffer = bbCreate(bufsize);
    if (buffer == NULL){
        fprintf(stderr, "Error in creating jbuffer\n");
        exit(EXIT_FAILURE);
    }

    // start thread pool
    pthread_t tid[threads];
    for (int i = 0; i < threads; i++){ // threads inherit sigmask
        errno = pthread_create(&tid[i], NULL, startRoutine, NULL);
        if (errno){
            // unnoetig, die() gibt Ressourcen sowieso frei
            /*int errnosave = errno;
            bbDestroy(buffer);
            errno = errnosave;*/
            die("pthread_create");
        }
    }

    // doch nicht, ist ja Endlosschleife
    // for (int i = 0; i < threads; i++){
    //     errno = pthread_join(&tid[i], NULL);
    //     if (errno){
    //         die("pthread_create");
    //     }
    // }
    // bbDestroy(buffer);
    return 0;
}


void handleConnection(int clientSock, int listenSock){

}