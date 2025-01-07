/* Öffentliche Beispiel-Implementierung des "sem" Moduls, indem diese auf POSIX
 * Semaphoren reduziert werden. Dies erlaubt die Nutzung von helgrind:
 * https://valgrind.org/docs/manual/hg-manual.html
 *
 * Das hier ist keine gültige Implementierung für den in SP2 zu
 * implementierenden Semaphor. */

#include <assert.h>
#include <errno.h>
#include <semaphore.h>
#include <stdlib.h>

#include "sem.h"

struct SEM {
  sem_t sem;
};

SEM *semCreate(int initVal) {
  SEM *sem;
  if (initVal < 0) {
    errno = EINVAL;
    return NULL;
  }
  if (NULL == (sem = malloc(sizeof(*sem)))) return NULL;
  if (-1 == sem_init(&sem->sem, 0, (unsigned)initVal))
    return sem_destroy(&sem->sem), NULL;
  return sem;
}

void semDestroy(SEM *sem) {
  if (sem == NULL) {
    return;
  }
  int err = sem_destroy(&sem->sem);
  assert(!err);
  free(sem);
}

void P(SEM *sem) {
  /* Signal handlers should be configured to restart syscalls. Thus not putting
   * an EINTR loop in here. */
  int err = sem_wait(&sem->sem);
  assert(!err);
}

void V(SEM *sem) {
  int err = sem_post(&sem->sem);
  assert(!err);
}
