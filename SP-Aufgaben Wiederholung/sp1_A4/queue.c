#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"
#include "sem.h"

typedef struct element {
    char *cmd;
    char *out;
    int flags;
    struct element *next;
} element;

static SEM *sync_sem;
static SEM *counter_sem;
element *head;

int queue_init(void) {
    sync_sem = semCreate(1);
    if (!sync_sem) return -1;
    counter_sem = semCreate(0);
    if (!counter_sem){
        semDestroy(sync_sem);
        return -1;
    }
    return 0;
}

void queue_deinit(void) {
    semDestroy(sync_sem);
    semDestroy(counter_sem);
    element *el = head;
    element *next;
    while (el){
        next = el->next;
        free(el);
        el = next;
    }
}

int queue_put(char *cmd, char *out, int flags) {
    P(sync_sem);
    if (!head){
        head = malloc(sizeof(element));
        if (!head){
            V(sync_sem);
            return -1;
        }
        head->cmd = cmd;
        head->out = out;
        head->flags = flags;
        head->next = NULL; // very important
        V(counter_sem);
        V(sync_sem);
        return 0;
    }
    element *el = head;
    while (el->next){
        el = el->next;
    }
    el->next = malloc(sizeof(element));
    if (!el->next){
        V(sync_sem);
        return -1;
    }     
    (el->next)->cmd = cmd;
    (el->next)->out = out;
    (el->next)->flags = flags;
    (el->next)->next = NULL; // very important
    V(sync_sem);
    V(counter_sem);
    return 0;
}

int queue_get(char **cmd, char **out, int *flags) {
    // order of P important (deadlock)
    P(counter_sem);
    P(sync_sem);
    if (!head){
        V(sync_sem);
        V(counter_sem);
        return -1;
    }
    *cmd = head->cmd;
    *out = head->out;
    *flags = head->flags;
    element *el = head;
    head = head->next;
    V(sync_sem);   
    free(el);
    return 0;
}