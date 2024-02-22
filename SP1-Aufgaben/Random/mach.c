#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "sem.h"
#include "run.h"
#include "queue.h"

struct param {
	SEM *sem;
	FILE *machfile;
        char cmd[4096+1]; // 4096 chars + '\0'
        int threads;      // only used by output_thread
};

static void die(const char *s) {
    perror(s);
    exit(EXIT_FAILURE);
}

static int parse_positive_int_or_die(char *str) {
    errno = 0;
    char *endptr;
    long x = strtol(str, &endptr, 10);
    if (errno != 0) {
        die("invalid number");
    }
    // Non empty string was fully parsed
    if (str == endptr || *endptr != '\0') {
        fprintf(stderr, "invalid number\n");
        exit(EXIT_FAILURE);
    }
    if (x <= 0) {
        fprintf(stderr, "number not positive\n");
        exit(EXIT_FAILURE);
    }
    if (x > INT_MAX) {
        fprintf(stderr, "number too large\n");
        exit(EXIT_FAILURE);
    }
    return (int)x;
}

static void *make(void *arg){
    // TODO:
    // make usage of errno, different codes
    // is an array allowed as an argument for a char* ?

    errno = pthread_detach(pthread_self());
    if (errno){
        die("pthread_detach");
    }
    struct param *par = arg;
    while (1) { // feof nochmal benutzen?
        P(par->sem);

        if (fgets(par->cmd, 4097, par->machfile) == NULL){
            // EOF or error
            V(par->sem); // Vree, so that every thread can check on feof, queue_put(deadpill) and break when work is done
            if (feof(par->machfile)){
                if (queue_put(par->cmd, NULL, 4)){ // flag "4" means "work finished"
                    perror("queue_put");
                    exit(EXIT_FAILURE);
                }
                break;
            } else {
                // ich bin mir nicht sicher ob Thread auf "die"-Fkt zugreifen kann
                perror("fgets");
                exit(EXIT_FAILURE);
            }
        }

        if (strcmp(par->cmd, "\n") == 0){ // new group of commands begins -> pause until all commands in current group are finished
            if (queue_put(par->cmd, NULL, 3)){ // flag "3" means "pause"
                perror("queue_put");
                exit(EXIT_FAILURE);
            }
            continue; // do not unlock the sem, but wait until output-thread unlocks it
        }

        V(par->sem);

        if (queue_put(par->cmd, NULL, 1)){ // flag "1" means "running"
            perror("queue_put");
            exit(EXIT_FAILURE);
        }

        char **out = NULL; // will automatically be allocated in run_cmd
	run_cmd(par->cmd, out); // ignore all errors!
	if (queue_put(par->cmd, *out, 2)) { // flag "2" means "completed"
	    die("queue_put");
	}
/*      int exitcode = run_cmd(par->cmd, out);
        if (exitcode){
            if (exitcode < 0){
                die("run_cmd");
            } else { // errno might not be set
                fprintf(stderr, "Error in run_cmd");
                exit(EXIT_FAILURE);
            }
        } else { // maybe out can be NULL when no Output? but I dont't think so
            if (out == NULL){ // not necessary I suppose, because exitcode is set, but safety first
                fprintf(stderr, "Error in run_cmd");
                exit(EXIT_FAILURE);
            }
            if (queue_put(par->cmd, out, 2)){ // flag "2" means "completed"
                die("queue_put");
            }
        }*/
    }


    return NULL;
}

static void *print(void *arg){
    // TODO:
    // make usage of errno, different codes
    // look at errno if exit_failure or not!
    // error handling queue_get
    // insert backslash n after printing -> maybe remove the backslash n from cmd

    errno = pthread_detach(pthread_self());
    if (errno){
        die("pthread_detach");
    }
    struct param *par = arg;

    char *cmd, *out;
    int flags;
    int deadpill_counter = 0;

    while (1){
        if (queue_get(&cmd, &out, &flags)) { // blocks until element available
        // error handling ...
        }
        if (flags == 1){
            printf("Running %s ...", cmd);
        } else if (flags == 2){
            printf("Completed %s: %s.", cmd, out);
        } else if (flags == 3){
            V(par->sem);        // new group of commands can begin, all commands of current group are finished
        } else if (flags == 4){
            deadpill_counter++; // one worker thread has finished
        }
        if (fflush(stdout)){
            die("fflush");
        }

        free(out);
        if (deadpill_counter == par->threads){ // all workers have finished
            break;
        }
    }
    // clean up
    free(cmd);
    free(out);
    semDestroy(par->sem);
    queue_deinit();

    return NULL;
}

int main(int argc, char **argv){
    if (argc != 3){
	    fprintf(stderr, "Usage: ./mach <threads> <mach-file>\n");
	    exit(EXIT_FAILURE);
    }

    int threads = parse_positive_int_or_die(argv[1]);
    FILE *file = fopen(argv[2], "r"); // open file in reading mode
    if (file == NULL){
        die("fopen");
    }

    SEM *sem = semCreate(1); // semaphore for reading file (only one thread at a time)
    if (sem == NULL){
        die("semCreate");
    }

    pthread_t workers[threads]; // #threads worker-threads +1 output thread
    pthread_t output;
    struct param args[threads + 1]; // structs for workers + output thread

    if (queue_init()){
        die("queue_init"); // errno has been set
    }

    for (int i = 0; i < threads; i++){ // start workers, jeder bekommt eigenes struct und alle teilen sich Semaphore
        args[i].sem = sem;
        args[i].machfile = file;
        errno = pthread_create(&workers[i], NULL, make, &args[i]);
        if (errno) {
            die("pthread_create");
        }
    }
    // Output-Thread bearbeitet print-Fkt und muss #threads wissen
    args[threads].sem = sem;
    args[threads].threads = threads;
    errno = pthread_create(&output, NULL, print, &args[threads]);
    if (errno) {
        die("pthread_create");
    }


/*
    // join workers
    for (int i = 0; i < threads; i++){
        errno = pthread_join(workers[i], NULL);
        if (errno) {
            die("pthread_join");
        }     
    }
    // join Output-Thread
    errno = pthread_join(output, NULL);
    if (errno) {
        die("pthread_join");
    } 
*/
    
//    exit(EXIT_SUCCESS); // weglassen, da main schneller beendet als workers?
}
