/*
 * file:        qthread.h
 * description: assignment - simple emulation of POSIX threads
 * class:       CS 5600, Fall 2019
 */
#ifndef __QTHREAD_H__
#define __QTHREAD_H__

#include <ucontext.h>           /* you'll need this */
#include <stdbool.h>

#define STACK_SIZE 64*1024
/* this is your qthread structure.
 */
typedef struct qthread {
    ucontext_t      ctx; //for context switching
    /* ... your code here */
    struct qthread *next;
    unsigned long sleepTime;
    bool done; //boolean flag indicating whether qthread_exit has been called
    void* returnValue;
    struct qthread *waiting; //pointer to thread waiting in qthread_join
    /*
     * REVIEW(ABHIJITH) : Might need to remove any of the below in the future
     */
    void *stack;
} qthread_t;

/* You'll need some sort of way of keeping lists and queues of threads.
 * suggestion:
 *  - add a 'next' pointer to the thread structure
 *  - create a queue structure, and 'push_back' and 'pop_front'
 *    functions that add and remove thread structures from the queue
 */
typedef struct thread_q {
    /* your code here */
    qthread_t *head;
    qthread_t *tail;
}qthread_list;



/* Mutex and cond structures - @allocate them in qthread_mutex_create /
 * qthread_cond_create and free them in @the corresponding _destroy functions.
 */
typedef struct qthread_mutex {
    bool locked;
    qthread_list waitlist;
} qthread_mutex_t;

typedef struct qthread_cond {
    qthread_list waitlist;
} qthread_cond_t;

/* You'll need to cast the function argument to makecontext
 * to f_void_t
 */
typedef void (*f_void_t)(void);

/* prototypes - see qthread.c for function descriptions
 */

void qthread_init(void);
qthread_t *qthread_create(void* (*f)(void*), void *arg1);
void qthread_yield(void);
void qthread_exit(void *val);
void *qthread_join(qthread_t *thread);
qthread_mutex_t *qthread_mutex_create(void);
void qthread_mutex_lock(qthread_mutex_t *mutex);
void qthread_mutex_unlock(qthread_mutex_t *mutex);
void qthread_mutex_destroy(qthread_mutex_t *mutex);
qthread_cond_t *qthread_cond_create(void);
void qthread_cond_wait(qthread_cond_t *cond, qthread_mutex_t *mutex);
void qthread_cond_signal(qthread_cond_t *cond);
void qthread_cond_broadcast(qthread_cond_t *cond);
void qthread_cond_destroy(qthread_cond_t *cond);
void qthread_usleep(long int usecs);
bool isEmpty(qthread_list *queue);
void push_back(qthread_list *queue, qthread_t *thread);
qthread_t* pop_front(qthread_list *queue);

#endif