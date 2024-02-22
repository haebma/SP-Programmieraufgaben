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
/*I----> +--------------------------------------------------------------------+
         | Die globl. Variable errno darf von Bibliotheksfunktionen niemals   |
         | auf den Wert 0 gesetzt werden. POSIX gibt zwar nur die Garantie    |
         | das keine POSIX-Funktion die errno auf 0. Im C11-Standard heißt es |
         | jedoch explizit: "[errno] is never set to zero by any library      |
         | function". (C11 §7.5 Abs. 3)                                       |
         +-------------------------------------------------------------------*/
    // also always returns 0
    errno = pthread_cond_init(&sem->cond, NULL);
/*I----> +--------------------------------------------------------------------+
         | Unter Linux können die beiden Aufrufe `pthread_cond_init` und      |
         | `pthread_mutex_init` zwar nicht fehlschlagen, in anderen POSIX-    |
         | Systemen schaut das unter Umständen jedoch durchaus anders aus     |
         | (siehe pthread_mutex_destroy(3p)). Schöner wäre es daher sich an   |
         | den POSIX-Manpages zu orientieren und eventuelle Fehler zu         |
         | behandeln. Falsch liegt man damit auch unter Linux nicht.          |
         +-------------------------------------------------------------------*/
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
/*I----> +--------------------------------------------------------------------+
         | Das ist keine sinnvolle Art um `pthread_mutex_destroy`             |
         | fehlerzubehandeln. Durch das `pthread_cond_broadcast` werden ja    |
         | höchstens Threads geweckt, die sich dann auch wieder mit um den    |
         | Mutex bewerben. Auch das unlock ohne ein vorhergegangenes lock im  |
         | selben Kontrollfluss scheint mir höchst fragwürdig. Das führt im   |
         | Zweifel zu sehr ungewöhnlichem und schwer untersuchbarem           |
         | Fehlerverhalten. Hier wäre es ok gewesen eine Fehlerbehandlung     |
         | auszulassen. `semDestroy` sollte nicht vom                         |
         | Bibliotheksnutzer/-nutzerin aufgerufen werden bevor nicht alle     |
         | Arbeiten an der Semaphore abgeschlossen sind, sonst ist das eben   |
         | "undefined behaviour". (-1)                                        |
         +-------------------------------------------------------------------*/

    while(errno = pthread_cond_destroy(&sem->cond)){
        // free all threads that still wait for cond
        pthread_cond_broadcast(&sem->cond);
    }
/*I----> +--------------------------------------------------------------------+
         | Auch diese Fehlerbehandlung ist ungeeignet und führt bestenfalls   |
         | zu einer Endlosschleife und schlechtesten Falls zu sehr komplexen  |
         | Fehlerverhalten (siehe oben). (-1)                                 |
         +-------------------------------------------------------------------*/
    free(sem);
}

// atomic function
void P(SEM *sem){
    errno = pthread_mutex_lock(&sem->mutex);
/*I----> +--------------------------------------------------------------------+
         | Die globl. Variable errno darf von Bibliotheksfunktionen niemals   |
         | auf den Wert 0 gesetzt werden. POSIX gibt zwar nur die Garantie    |
         | das keine POSIX-Funktion die errno auf 0. Im C11-Standard heißt es |
         | jedoch explizit: "[errno] is never set to zero by any library      |
         | function". (C11 §7.5 Abs. 3)                                       |
         +-------------------------------------------------------------------*/
/*
 * Da die Methode nur bei falschen Parametern fehlschlägt (und die hier nicht direkt vom Nutzer kommen) könnte man die Fehlerbehandlung hier auch weglassen. (Ebenso unten bei unlock)
 */
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

/*P----> +--------------------------------------------------------------------+
         | Punktabzug in dieser Datei: 2.0 Punkte                             |
         +-------------------------------------------------------------------*/
