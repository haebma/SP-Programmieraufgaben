#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#include "queue.h"
#include "sem.h"
#include "run.h"

#define CMD_LENGTH 4096
#define FLAG_RUNNING 1
#define FLAG_COMPLETED 2
#define FLAG_WORK_FINISHED 3 // poison pill

static FILE *rx;
static SEM *read_sem;
static SEM *group_sem;
static volatile bool ret = false; // join and restart threads after each group of cmds
static volatile bool done = false; // join and terminate program

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

static void *work(){
    char cmd[CMD_LENGTH + 1];
    for (;;){
        // read cmd
        P(read_sem);
        if(ret){
            // group done
            V(read_sem);
            V(group_sem); // signal break to main()
            return NULL;
        }
        if (!fgets(cmd, sizeof(cmd), rx)){
            // EOF or error
            if (feof(rx)){
                done = true;
                V(read_sem);
                V(group_sem); // signal termination to main()
                return NULL; // work done completely
            } else {
                fprintf(stderr, "Error fgets\n");
                exit(EXIT_FAILURE);
            }
        }
        if (strlen(cmd) == 1 && cmd[0] == '\n'){
            // signal group done
            ret = true;
            V(read_sem);
            V(group_sem);
            return NULL;
        }
        // 'normal' command
        V(read_sem);

        // remove '\n' if necessary
        if (cmd[strlen(cmd) - 1] == '\n') cmd[strlen(cmd) - 1] = '\0';

        // put cmd in queue
        char *cmd_dup = strdup(cmd);
        if (!cmd_dup) die("strdup");
        if (queue_put(cmd_dup, NULL, FLAG_RUNNING)) die("queue_put");

        // run cmd
        char *out;
        if (run_cmd(cmd, &out) < 0) die("run_cmd");
        
        // put output in queue
        if (queue_put(cmd_dup, out, FLAG_COMPLETED)) die("queue_put");
    }
}

static void *print(){
    for (;;){
        char *cmd = "Dummy_cmd";
        char *out = "Dummy_out";
        int flags;
        if (queue_get(&cmd, &out, &flags)) die("queue_get"); // blocks until available
        if (flags == FLAG_RUNNING){
            printf("Running '%s' ...\n", cmd);          
        } else if (flags == FLAG_COMPLETED){
            printf("Completed '%s': \"%s\"\n", cmd, out);
            free(cmd);
            free(out);
        } else if (flags == FLAG_WORK_FINISHED){
            return NULL;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3){
        fprintf(stderr, "usage: ./mach <anzahl threads> <mach-datei>\n");
        exit(EXIT_FAILURE);
    }
    int threadc = parse_positive_int_or_die(argv[1]);
    pthread_t tids[threadc];
    pthread_t printer;
    if (queue_init()) die("queue_init");
    read_sem = semCreate(1);
    if (!read_sem) die("semCreate");
    group_sem = semCreate(0);
    if (!group_sem) die("semCreate");
    rx = fopen(argv[2], "r");
    if (!rx) die("fopen");

    // create threads
    for (int i = 0; i < threadc; i++){
        if (errno = pthread_create(&tids[i], NULL, &work, NULL)) die("pthread_create");
        if (errno = pthread_detach(tids[i])) die("pthread_detach");
    }
    if (errno = pthread_create(&printer, NULL, &print, NULL)) die("pthread_create");
    
    // join and restart threads for every group
    for(;;){
        for (int i = 0; i < threadc; i++){
            P(group_sem);
        }
        if (done) break;
        ret = false;
        for (int i = 0; i < threadc; i++){
            if (errno = pthread_create(&tids[i], NULL, &work, NULL)) die("pthread_create");
            if (errno = pthread_detach(tids[i])) die("pthread_detach");
        }
    }

    // signal termination to printer-thread
    if (queue_put(NULL, NULL, FLAG_WORK_FINISHED)) die("queue_put");

    // clean up
    if (pthread_join(printer, NULL)) die("pthread_join");
    fflush(stdout);
    fclose(rx);
    queue_deinit();
    semDestroy(read_sem);
    semDestroy(group_sem);
    exit(EXIT_SUCCESS);
}
// TODO:
// nothing, task complete :-)