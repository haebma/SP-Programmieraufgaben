#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "sem.h"

typedef struct BNDBUF BNDBUF;

struct BNDBUF {
    size_t array_size;
    size_t put_index; // only accessed by producer-thread
    volatile _Atomic int take_index; // only accessed by consumer-threads
    int *array;
    SEM *sem_put;
    SEM *sem_take;
};

// free bb itself before return?
BNDBUF *bbCreate(size_t size){
    if (size <= 0) return NULL;
    
    BNDBUF *bb = (BNDBUF*) malloc(sizeof(BNDBUF));
    if (bb == NULL) return NULL;
    bb->array_size = size;
    bb->put_index = 0;
    atomic_init(&bb->take_index, 0);
    bb->array = (int*) malloc(size * sizeof(int));
    if (bb->array == NULL) return NULL;
    
    bb->sem_put = semCreate(size);
    if (bb->sem_put == NULL) {
        free(bb->array);
        free(bb);
        return NULL;
    }

    bb->sem_take = semCreate(0);
    if (bb->sem_take == NULL){
        semDestroy(bb->sem_put);
        free(bb->array);
        free(bb);
        return NULL;
    }

    return bb;
}

void bbDestroy(BNDBUF *bb){
    if (bb == NULL) return;

    free(bb->array);
    semDestroy(bb->sem_put);
    semDestroy(bb->sem_take);
    free(bb);
}

// only one producer-thread -> no mutual exclusion needed
void bbPut(BNDBUF *bb, int value){
    if (bb == NULL) return;
    // Typ allgemeiner Semaphore
    P(bb->sem_put);
    bb->array[bb->put_index] = value;
    V(bb->sem_take);
    bb->put_index = (bb->put_index + 1) % bb->array_size;
}

// consumer-threads shall not block each other!
int bbGet(BNDBUF *bb){
    int value = -1;
    int idx = -1;
    int new_idx = -1;
    int bufsize = bb->array_size;
    P(bb->sem_take);
    // the following block shall be atomic
    do {
        idx = atomic_load(&bb->take_index); // load index
        value = bb->array[idx]; // load value at index
        new_idx = (idx + 1) % bufsize;
        // if no thread has modified the index in between, the block was completed atomically -> increment take_index
    } while(atomic_compare_exchange_strong(&bb->take_index, &idx, new_idx) == false);
    V(bb->sem_put);
    return value;
}