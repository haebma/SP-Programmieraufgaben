#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

typedef struct SEM SEM;

struct SEM {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};
// TODO: is this a correct way to set errno? (implicitely via malloc, etc)
SEM *semCreate(int initVal){
    SEM *sem = (SEM*) malloc(sizeof(SEM));
    if (sem == NULL) return NULL;
    // always returns 0
    errno = pthread_mutex_init(&sem->mutex, NULL);
    // also always returns 0
    errno = pthread_cond_init(&sem->cond, NULL);    
    sem->value = initVal;
    return sem;
}

// TODO: make atomic, so that no race condition can occurr
void semDestroy(SEM *sem){
    if (sem == NULL) return;
    
    while (errno = pthread_mutex_destroy(&sem->mutex)){
        pthread_cond_broadcast(&sem->cond);
        pthread_mutex_unlock(&sem->mutex);
    }
    
    while(errno = pthread_cond_destroy(&sem->cond)){
        // free all threads that still wait for cond
        pthread_cond_broadcast(&sem->cond);
    }
    free(sem);
}

// atomic function
void P(SEM *sem){
    errno = pthread_mutex_lock(&sem->mutex);
    if (errno) return;
    while (sem->value <= 0){
        pthread_cond_wait(&sem->cond, &sem->mutex); // implicitely unlocks the mutex, waits and then locks it again
    }
    sem->value--;
    errno = pthread_mutex_unlock(&sem->mutex);
}

// atomic function
void V(SEM *sem){
    pthread_mutex_lock(&sem->mutex);
    sem->value++;
    if (sem->value > 0){
        pthread_cond_broadcast(&sem->cond); // restarts all waiting threads
    }
    pthread_mutex_unlock(&sem->mutex);
}