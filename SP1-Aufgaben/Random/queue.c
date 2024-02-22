#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "sem.h"

// TODO:
//       - when semDestroy??
//       - AUF WAS SOLL ERRNO GESETZT WERDEN?
//       - queue_get pointer mallocen?
//       - queue_get no errors possible?

struct element {
    char *cmd;
    char *out;
    int flags;
    struct element *next;
};

static struct element *head;
static SEM *sem1; // for adding and removing entries (one at a time)
static SEM *sem2; // for removing values out of the queue; works as an element counter



/**
 * @brief Initialize the queue.
 *
 * This function initializes the queue. It's UNDEFINED to call other functions
 * of this module before initializing the queue.
 *
 * If an error occurs during initialization, all already allocated resources
 * must be freed before returning an error.
 *
 * @return @c 0 on success, any other value on error (sets @c errno on error)
 */
int queue_init(void) { // race conditions in semCreate possible -> queue_init() must only be invoked by one Thread!
    if (head != NULL){ // queue is already initialized
        errno = 1;
        return -1;
    }
    
    sem1 = semCreate(1);
    sem2 = semCreate(0);
    if (sem1 == NULL || sem2 == NULL){ // error in semCreate
        semDestroy(sem1);
        semDestroy(sem2);
        errno = 2;
        return -1;
    }

    head = (struct element*) malloc(sizeof(struct element));
    if (head == NULL){ // error in malloc
        semDestroy(sem1);
        semDestroy(sem2);
        errno = 3;
        return -1;
    }

    return 0; // success
}


/**
 * @brief Destroy the queue.
 *
 * This functions deallocates all resources of the queue. queue_deinit()
 * assumes the queue is empty and will not remove existing elements. It's
 * undefined to call other functions of this module after calling this
 * function.
 */
void queue_deinit(void) { // assumes queue is empty!
    free(head); // no race conditions possible (?)
    semDestroy(sem1);
    semDestroy(sem2);
}


/**
 * @brief Add entry to the queue.
 *
 * Add a new entry with command @c cmd, output @c out and optional flags @c
 * flags to the queue. All values CAN BE @c NULL respectively @c 0. @c flags
 * is optional and can be used to store additional attributes.
 *
 * The caller controls the lifespan of all pointers. The queue will not
 * duplicate or copy any arguments. It only stores the provided pointers.
 *
 * @param cmd command to run
 * @param out output of the command
 * @param flags additional flags (optional)
 * @return @c 0 on success, any other value on error (sets @c errno on error)
 */
int queue_put(char *cmd, char *out, int flags) { // no copying just storing
    P(sem1);

    if (head == NULL){ // if queue_init() hasn't been called before
        errno = 1;
        V(sem1);
        return -1;
    }

    struct element *el = head;
    while (el->next != NULL){ // go to last element; head pointer stays empty in this implementation, stores no values
        el = el->next;
    }

    struct element *entry = (struct element *) malloc(sizeof(struct element));
    if (entry == NULL){ // error in malloc
        errno = 2;  // I think errno is set automatically in malloc here --------------------------
        V(sem1);
        return -1;
    }
    el->next = entry;
    entry->cmd = cmd;
    entry->out = out;
    entry->flags = flags;
    entry->next = NULL;

    V(sem2); // notificate that element has been added
    V(sem1); // in this order so that queue_get has the same chances as put
    return 0; // success
}


/**
 * @brief Remove entry from the queue.
 *
 * Removes an entry from the queue and writes its values to the given
 * pointers. The function WAITS until an element is available.
 *
 * Example:
 *
 * \code
 * char *cmd, *out;
 * int flags;
 * if (queue_get(&cmd, &out, &flags)) {
 *     // error handling ...
 * }
 * \endcode
 *
 * @param cmd command to run
 * @param out output of the command
 * @param flags additional flags (optional)
 * @return @c 0 on success, any other value on error (sets @c errno on error)
 */
int queue_get(char **cmd, char **out, int *flags) { // removes first element
    P(sem2); // decrement element count -> wait until element available
    P(sem1); // THEN block other threads (deadlock); necessary because of race condition in reading pointers
    
    /*if (head == NULL){ // queue hasn't been initialized (should never occur)
        errno = 1;
        return -1;
    } */

    struct element *entry = head->next;

    cmd = &(entry->cmd);
    out = &(entry->out);
    flags = &(entry->flags);
    head->next = entry->next;
    free(entry);

    V(sem1);
    return 0; // success
}
