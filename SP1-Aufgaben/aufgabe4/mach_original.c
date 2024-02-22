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
//TODO: P(par->sem) current-threads mal aufrufen, dabei bei jedem Aufruf von make mit anderer Semaphore bullshiiiit
struct param {
	SEM *sem;
	SEM *sem_end;
	FILE *machfile;
        char cmd[4096+1]; // 4096 chars + '\0'
        int threads;      // only used by output_thread
};

static SEM *sem_end;

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
    errno = pthread_detach(pthread_self());
    if (errno){
        die("pthread_detach");
    }
    struct param *par = arg;

    while (1) {
        P(par->sem);

        if (fgets(par->cmd, 4097, par->machfile) == NULL){
            // EOF or error
            V(par->sem); // Vree, so that every thread can check on feof, queue_put(poisonpill) and break when work is done
            if (feof(par->machfile)){
                if (queue_put(par->cmd, NULL, 4)){ // flag "4" means "work finished"
                    die("queue_put");
                }
                break;
            } else {
                die("fgets");
            }
        }
// TODO: does not work like this
        if (strcmp(par->cmd, "\n") == 0){ // new group of commands begins -> pause until all commands in current group are finished
            if (queue_put(par->cmd, NULL, 3)){ // flag "3" means "pause"
                die("queue_put");
            }
            continue; // do not unlock the sem, but wait until output-thread unlocks it
        }

        V(par->sem);

	par->cmd[strlen(par->cmd) - 1] = '\0'; // cut '\n' out
        if (queue_put(par->cmd, NULL, 1)){ // flag "1" means "running"
            die("queue_put");
        }

        char *out; // will automatically be allocated in run_cmd
	run_cmd(par->cmd, &out); // ignore all errors!
//printf("run_cmd ausgefuehrt:\n Command: %s\n Output: %s\n", par->cmd, out);
	if (queue_put(par->cmd, out, 2)) { // flag "2" means "completed"
	    die("queue_put");
	}

	
//printf("queue_put-Aufruf aus make-Fkt erfolgreich, Durchlauf komplett\n");
    }

    return NULL;
}

static void *print(void *arg){
    errno = pthread_detach(pthread_self());
    if (errno){
        die("pthread_detach");
    }
    struct param *par = arg;

//    char *cmd, *out;
    int flags;
    int poisonpill_counter = 0;

    while (1){
	char *cmd = NULL;
	char *out = NULL;
	if (queue_get(&cmd, &out, &flags)) { // blocks until element available
	    die("queue_get");
        }
	
// printf("Print-Fkt: Bekommene Werte aus queue_get:\n Command: %s\n Out: %s\n Flags: %d\n", cmd, out, flags);
        if (flags == 1){
            printf("Running %s ...\n", cmd);
        } else if (flags == 2){
            printf("Completed %s: \"%s\"\n", cmd, out);
        } else if (flags == 3){
            V(par->sem);        // new group of commands can begin, all commands of current group are finished
        } else if (flags == 4){
            poisonpill_counter++; // one worker thread has finished
        }
        if (fflush(stdout)){
            die("fflush");
        }
	if (cmd != NULL) free(cmd);
	if (out != NULL) free(out);

        if (poisonpill_counter == par->threads){ // all workers have finished
            break;
        }
    }
    // clean up
    semDestroy(par->sem);
    queue_deinit();
    V(par->sem_end);  // let main-Thread finish
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
    sem_end = semCreate(0);
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
	
//    printf("in for-Schleife, %d-ter Thread erfolgreich kreiert\n", i);
    }
    // Output-Thread bearbeitet print-Fkt und muss #threads wissen
    args[threads].sem = sem;
    args[threads].threads = threads;
    args[threads].sem_end = sem_end;
    errno = pthread_create(&output, NULL, print, &args[threads]);
    if (errno) {
        die("pthread_create");
    }

    P(sem_end);
    semDestroy(sem_end);
    if (fclose(file)){
	die("fclose");
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

//    exit(EXIT_SUCCESS);
}
