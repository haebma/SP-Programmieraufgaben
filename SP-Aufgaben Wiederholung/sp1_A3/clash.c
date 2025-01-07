#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "plist.h"


#define CWD_SIZE 30
#define MAX_LINE_LENGTH 1337

static void die(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

static void printcwd(){
    /* prints current working directory */
    size_t size = CWD_SIZE;
    char *cwd = malloc(size);
    if (!cwd) die("malloc");

    while (!getcwd(cwd, size)){
        if (errno == ERANGE){
            // allocate larger buffer
            size = 1.5*size;
            cwd = realloc(cwd, size);
            if (!cwd) die("realloc");
        } else {
            die("getcwd");
        }
    }
    
    printf("%s: ", cwd);
    free(cwd);
}

static int print_callback(pid_t pid, const char *cmd){
    if (printf("%d %s\n", pid, cmd) < 0) return -1;
    else return 0;
}

static void free_args(char **args, int args_counter){
    for (int i = 0; i < args_counter; i++){
        free(args[i]);
    }
    free(args);
}

static void read_n_execute_line(){
    // read
    char line[MAX_LINE_LENGTH + 1];
    if (!fgets(line, sizeof(line), stdin)){
        // Error or EOF without any characters read
        if (ferror(stdin)){
            die("fgets");
        } else {
            exit(EXIT_SUCCESS);
        }
    }
    if (line[strlen(line)-1] != '\n'){
        fprintf(stderr, "Overlarge line will be discarded\n");
        int c;
        while ((c = fgetc(stdin)) != EOF){
            if (c == '\n') break;
        }
        if (ferror(stdin)){
            fprintf(stderr, "Error in fgetc\n");
            exit(EXIT_FAILURE);
        }
        return;
    } else {
    // execute
        line[strlen(line) - 1] = '\0'; // overwrite '\n'
        bool background = false;
        if (line[strlen(line) - 1] == '&') {
            background = true;
            line[strlen(line) - 1] = '\0';
        }
        char line_cpy[strlen(line)+1];
        strcpy(line_cpy, line);
        
        int args_size = 2;
        int args_counter = 0;
        char **args = malloc(args_size*sizeof(char *));
        if (!args) die("malloc");
        
        // write command and all arguments of line into array
        for(int i = 0;;i++){
            char *arg;
            if (i == 0){
                // cmd
                arg = strtok(line_cpy, " \t\n"); // manipulates line_cpy(local char []) 4eva
                if (!arg) break;        
            } else {
                // args
                arg = strtok(NULL, " \t\n");
                if (!arg) break;
                if (i >= args_size){
                    args_size *= 2;
                    args = realloc(args, args_size*sizeof(char *));
                    if (!args) die("realloc");
                }
            }
            args[i] = strdup(arg);
            if (!args[i]) die("strdup");
            args_counter++;
        }

        if (!strcmp(args[0], "cd")){
            errno = 0;
            if (chdir(args[1])) perror("chdir");
            free_args(args, args_counter);
            return;
        }
        if (!strcmp(args[0], "jobs")){
            walkList(&print_callback);
            free_args(args, args_counter);
            return;
        }

        pid_t pid = fork();
        if (pid == -1){
            die("fork");
        } else if (pid == 0){
            // child process
            execvp(args[0], args);
            die("exec");
        } else {
            // parent process
            // free args and stuff

            if (background){
                // put into list
                if (insertElement(pid, line) == -2){
                    fprintf(stderr, "insertElement: insufficient memory\n");
                    exit(EXIT_FAILURE);
                }
                free_args(args, args_counter);
                return;
            } else {
                // foreground process: wait until process done
                int wstatus;
                if (waitpid(pid, &wstatus, 0) == -1) die("waitpid");
                printf("Exitstatus [%s]: %d\n", line, wstatus);
                free_args(args, args_counter);
            }
        }
    }
}

int main(){

    char cmd[MAX_LINE_LENGTH + 1];
    int wstatus;
    pid_t pid;
    for(;;){
        // Zombies aufsammeln
        wstatus = 0;
        errno = 0;
        if ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0){
            if (removeElement(pid, cmd, sizeof(cmd)) == -1){
                fprintf(stderr, "removeElement: pair with given pid does not exist\n");
                exit(EXIT_FAILURE);
            }
            printf("Exitstatus [%s]: %d\n", cmd, wstatus);
        } else if (pid == -1){
            if (!(errno == ECHILD)) die("waitpid");
        }

        printcwd();
        read_n_execute_line();
    }
}

// TODO:
// nothing, task complete :)