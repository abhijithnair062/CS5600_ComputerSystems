/*
 * file:        qthread.c
 * description: assignment - simple emulation of POSIX threads
 * class:       CS 5600, Fall 2019
 */

/* a bunch of includes which will be useful */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <errno.h>
#include <ucontext.h>
#include <stdarg.h>

#include "qthread.h"


/*
 * current -> current thread
 * mainThread -> CPU's thread
 * active -> list of active threads
 */
qthread_t *current;
qthread_t *mainThread;
qthread_list active;
qthread_list sleepers;
/* Helper function for the POSIX replacement API - you'll need to tell
 * time in order to implement qthread_usleep. WARNING - store the
 * return value in 'unsigned long' (64 bits), not 'unsigned' (32 bits)
 */
static unsigned long get_usecs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000 + tv.tv_usec;
}

/*
 * This function checks if the queue list is empty
 */
bool isEmpty(qthread_list *queue){
    if(queue->head == NULL){
        return true;
    }
    return false;
}
/*
 * Function to add a thread to the back of the queue
 */
void push_back(qthread_list *queue, qthread_t *thread){
    thread->next = NULL;
    if(isEmpty(queue)){
        queue->head = thread;
        queue->tail = thread;
    }else{
        queue->tail->next = thread;
        queue->tail = thread;
    }
}
/*
 * Function to pop thread from the queue
 */
qthread_t* pop_front(qthread_list *queue){
    if(isEmpty(queue)){
        return NULL;
    }
    qthread_t *temp = queue->head;
    queue->head = temp->next;
    if(queue->head == NULL){
        queue->tail = NULL;
    }
    temp->next = NULL;
    return temp;
}


/* wrapper function:
 *  - call f(arg)
 *  - if call returns, pass return value to qthread_exit
 */
void qthread_wrapper(void* (*f)(void*), void *arg)
{
    /* your code here */
    void *result = f(arg);
    qthread_exit(result);
}



/* qthread_create - see hints for how to implement it, especially:
 * - using getcontext and makecontext
 * - using a "wrapper" function to capture return value of f(arg1)
 *
 * note that the crazy definition of the first argument means:
 *   f is a pointer to a function with args=(void*), returning void*
 */
qthread_t *qthread_create(void* (*f)(void*), void *arg1)
{
    //Initialize thread and stack
    qthread_t *thread = malloc(sizeof(*thread));
    thread->stack = malloc(STACK_SIZE);
    thread->done = false;
    thread->next = NULL;
    thread->sleepTime = 0;
    thread->returnValue = NULL;
    thread->waiting = NULL;
    memset(thread->stack, 0xA5, STACK_SIZE);
    thread->ctx.uc_stack = (stack_t) {
            .ss_sp = thread->stack,
            .ss_flags = 0,
            .ss_size = STACK_SIZE
    };
    makecontext(&thread->ctx, (f_void_t) qthread_wrapper,2,f,arg1);
    push_back(&active,thread);
    return thread;
}

void schedule(void)
{
    /* I suggest factoring your code so that you have a 'schedule'
     * function which selects the next thread to run and @switches to it,
     * or goes to sleep if there aren't any threads left to run.
     *
     * NOTE - if you end up switching back to the same thread, do *NOT*
     * use swapcontext - check for this case and return from schedule(),
     * or else you'll crash.
     */
    /*
    * Traverse all  threads sleeping
    * If thread sleep time has timed out they are added to active list else they are added to tempList
    */
    do{
        qthread_list *tempList = calloc(sizeof(*tempList),1);
        tempList->head = NULL;
        tempList->tail = NULL;
        qthread_t *temp = pop_front(&sleepers);
        while(temp != NULL){
            if(temp->sleepTime <= get_usecs()){
                push_back(&active,temp);
            }else{
                push_back(tempList,temp);
            }
            temp = pop_front(&sleepers);
        }

        /*
         * Now move all threads from tempList to activeList
         */
        temp = pop_front(tempList);
        while(temp != NULL){
            push_back(&sleepers,temp);
            temp = pop_front(tempList);
        }
        free(tempList);
    }while(isEmpty(&active) && !isEmpty(&sleepers));

    qthread_t *self = current;
    current= pop_front(&active);

    if(current == NULL){
        current = mainThread;
    }
    if(current == self){
        return;
    }
    swapcontext(&self->ctx,&current->ctx);
}

/* qthread_init - set up a thread structure for the main (OS-provided) thread
 */
void qthread_init(void)
{
    mainThread= calloc(sizeof(*mainThread),1);
    mainThread->next = NULL;
    mainThread->sleepTime = 0;
    mainThread->done = false;
    mainThread->returnValue = 0;
    mainThread->waiting = NULL;
    getcontext(&mainThread->ctx);
    current = mainThread;
    /*
     * Initialize active list, sleepers list and waiter list
     */
    active.head = NULL;
    active.tail = NULL;
    sleepers.head = NULL;
    sleepers.tail = NULL;
}

/* qthread_yield - yield to the next @runnable thread.
 */
void qthread_yield(void)
{
    if(current != NULL && current != mainThread){
        push_back(&active,current);
    }
    schedule();
}

/* qthread_exit, qthread_join - exit argument is returned by
 * qthread_join. Note that join blocks if the thread hasn't exited
 * yet, and is allowed to crash @if the thread doesn't exist.
 */
void qthread_exit(void *val)
{
    qthread_t *self = current;
    self->returnValue = val;
    self->done = true;
    if(self->waiting){
        push_back(&active,self->waiting);
    }
    schedule();
}

void *qthread_join(qthread_t *thread)
{
    if(!thread->done){
        thread->waiting = current;
        schedule();
    }
    void* val = thread->returnValue;
    free(thread->stack);
    free(thread);
    return val;
}

/* Mutex functions
 */
qthread_mutex_t *qthread_mutex_create(void)
{
    qthread_mutex_t *mutex = malloc(sizeof(qthread_mutex_t));
    mutex->locked = false;
    mutex->waitlist.head = NULL;
    mutex->waitlist.tail = NULL;
    return mutex;
}

void qthread_mutex_destroy(qthread_mutex_t *mutex)
{
    assert(mutex->locked == false);
    free(mutex);
}

void qthread_mutex_lock(qthread_mutex_t *mutex)
{
    if (!mutex->locked)
    {
        mutex->locked = true;
    }
    else
    {
        push_back(&(mutex->waitlist), current);
        schedule();
    }
}
void qthread_mutex_unlock(qthread_mutex_t *mutex)
{
    if (isEmpty(&mutex->waitlist)){
        mutex->locked = false;
    }
    else
    {
        qthread_t *tmp = pop_front(&mutex->waitlist);
        push_back(&active, tmp);
    }
}

/* Condition variable functions
 */
qthread_cond_t *qthread_cond_create(void)
{
    qthread_cond_t *cond= calloc(sizeof(*cond),1);
    cond->waitlist.head = NULL;
    cond->waitlist.tail = NULL;
    return cond;
}
void qthread_cond_destroy(qthread_cond_t *cond)
{
    free(cond);
}
void qthread_cond_wait(qthread_cond_t *cond, qthread_mutex_t *mutex)
{
    push_back(&cond->waitlist,current);
    qthread_mutex_unlock(mutex);
    schedule();
    qthread_mutex_lock(mutex);
}
void qthread_cond_signal(qthread_cond_t *cond)
{
    qthread_t *temp;
    temp = pop_front(&cond->waitlist);
    if(temp != NULL){
        push_back(&active,temp);
    }
}
void qthread_cond_broadcast(qthread_cond_t *cond)
{
    qthread_t *temp = pop_front(&cond->waitlist);
    while(temp != NULL){
        push_back(&active,temp);
        temp = pop_front(&cond->waitlist);
    }
}


/* POSIX replacement API. This semester we're only implementing 'usleep'
 *
 * If there are no runnable threads, your scheduler needs to block
 * waiting for a thread blocked in 'qthread_usleep' to wake up.
 */

/* qthread_usleep - yield to next runnable thread, making arrangements
 * to be put back on the active list after 'usecs' timeout.
 */
void qthread_usleep(long int usecs)
{
    qthread_t *toSleep = current;
    toSleep->sleepTime = get_usecs() + usecs;
    push_back(&sleepers,toSleep);
    schedule();
}