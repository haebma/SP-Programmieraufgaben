#include "pthread.h"
#include "queue.h"
#include "sem.h"
#include "run.h"
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static void die(const char *s)
{
    perror(s);
    exit(EXIT_FAILURE);
}

static int parse_positive_int_or_die(char *str)
{
    errno = 0;
    char *endptr;
    long x = strtol(str, &endptr, 10);
    if (errno != 0)
    {
        die("invalid number");
    }
    // Non empty string was fully parsed
    if (str == endptr || *endptr != '\0')
    {
        fprintf(stderr, "invalid number\n");
        exit(EXIT_FAILURE);
    }
    if (x <= 0)
    {
        fprintf(stderr, "number not positive\n");
        exit(EXIT_FAILURE);
    }
    if (x > INT_MAX)
    {
        fprintf(stderr, "number too large\n");
        exit(EXIT_FAILURE);
    }
    return (int)x;
}

#define MAX_LINE_LENGTH 4096
#define QUEUE_FLAG_CMD_RUNNING 0
#define QUEUE_FLAG_CMD_COMPLETED 1
#define QUEUE_FLAG_POISONPILL 9

static SEM * max_threads_semaphore;
static pthread_t * current_threads = NULL;
static char ** current_threads_cmds = NULL;
static int current_threads_count = 0;

static void *output_thread()
{
    char * cmd = NULL;
    char * out = NULL;
    int flags = 0;

    while (1)
    {
        int get_err = queue_get(&cmd, &out, &flags);
        if(get_err) {
            return (void *) 1;
        }

        if(flags == QUEUE_FLAG_CMD_RUNNING) {
            printf("Running `%s` ...", cmd);
            fflush(stdout);
        } else if(flags == QUEUE_FLAG_CMD_COMPLETED) {
            printf("Completed `%s`: \"%s\"", cmd, out);
            fflush(stdout);
        } else {
            break;
        }

        if(out != NULL) {
            free(out); // out has to be free()'d
        }
    }

    return (void *) 0;
}

static void *executor_thread(void * thread_argument) {
    char * cmd = (char *) thread_argument;
    char * output;
    queue_put(cmd, "", QUEUE_FLAG_CMD_RUNNING);
    run_cmd(cmd, &output);
    queue_put(cmd, output, QUEUE_FLAG_CMD_COMPLETED);

    return (void *) 0;
}

static void start_executor_thread(char* cmd) {
    P(max_threads_semaphore);

    current_threads_count++;

    // for timing reasons, the command has to be copied as well
    char * command_cpy = strdup(cmd);
    if(command_cpy == NULL) {
        die("Error while allocating memory for cmd");
    }

    current_threads_cmds = realloc(current_threads_cmds, current_threads_count * (sizeof(char *)));
    if(current_threads_cmds == NULL) {
        die("Error while allocating memory for cmd list");
    }
    current_threads_cmds[current_threads_count - 1] = command_cpy;

    // start thread
    pthread_t executor_thread_id;
    int stdout_executor_thread = pthread_create(&executor_thread_id, NULL, &executor_thread, command_cpy);
    if (stdout_executor_thread) {
        die("Failed to create a thread to run command");
    }

    // add thread to thread list
    current_threads = realloc(current_threads, current_threads_count * (sizeof(pthread_t)));
    if(current_threads == NULL) {
        die("Error while allocating memory for thread list");
    }
    current_threads[current_threads_count - 1] = executor_thread_id;
}

static void await_executor_thread(pthread_t thread_id) {
    pthread_join(thread_id, NULL);
    V(max_threads_semaphore);
}

static void process_file(char* path, int thread_count) {
    FILE * fp;
    char * command = NULL;
    size_t len = 0;
    ssize_t line_length;

    fp = fopen(path, "r");
    if (fp == NULL) {
        die("Could not open machfile.");
    }

    while ((line_length = getline(&command, &len, fp)) != -1) {
        if(command[0] == '\n') {
            // await all running threads
            for(int i = 0; i < current_threads_count; i++) {
                
                await_executor_thread(current_threads[i]);
            }

            // clear thread list
            current_threads = realloc(current_threads, 0);
            if(current_threads == NULL) {
                die("Error while reallocating memory for thread list");
            }
            
            // clear cmd list
            for(int i = 0; i < current_threads_count; i++) {
                free(current_threads_cmds[i]);
            }
            current_threads_cmds = realloc(current_threads_cmds, 0);
            current_threads_count = 0;

            continue;
        }

        start_executor_thread(command);
    }

    if(fclose(fp) != 0) {
        die("Machfile could not be closed successfully");
    }

    // signalize end of queue
    queue_put("", "", QUEUE_FLAG_POISONPILL);
}

int main(int argc, char **argv)
{
    // parse input
    if (argc != 3)
    {
        die("Invalid parameter count. Use ./mach <anzahl threads> <mach-datei>");
    }

    int thread_count = parse_positive_int_or_die(argv[1]);
    char *mach_file = argv[2];

    // create semaphore for thread count
    max_threads_semaphore = semCreate(thread_count);
    if(max_threads_semaphore == NULL) {
        die("Could not create semaphore to limit current threads");
    }

    // initialize queue
    if (queue_init() != 0) {
        die("Could not initalize message queue");
    }

    // create output thread
    pthread_t stdout_thread_id;
    int stdout_thread_err = pthread_create(&stdout_thread_id, NULL, &output_thread, NULL);
    if (stdout_thread_err) {
        die("Failed to create a thread to display messages to stdout");
    }

    process_file(mach_file, thread_count);

    // cleanup
    int stdout_thread_exitstatus;
    pthread_join(stdout_thread_id, (void **)&stdout_thread_exitstatus);
    if(stdout_thread_exitstatus != 0) {
        die("Output thread was ended incorrectly. It cannot be ensured that all messages have been issued!");
    }
    
    queue_deinit();
    semDestroy(max_threads_semaphore);
    free(current_threads);
    free(current_threads_cmds);
}
