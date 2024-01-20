Homework 2 - User-space thread library
============================

In this homework you will write a user-space thread system (named “qthreads”, vs. the standard “pthreads” library), which creates threads, performs context switches, and includes implementations of mutexes and condition variables. You will be responsible for writing a thorough test suite for your code.

Rules
-----

Unless you arrange otherwise, teams will be the same as for Programming Assignment 1. Feel free to discuss the ideas in the homework with other groups; however you must write answers to written questions in your own words, and absolutely no sharing of code across groups is allowed. You do not need to explicitly submit your assignment – your grade will be based on the code in your repository at the time it is due.

**Commit and push your code at the end of every day that you work on it**, or (preferably) more frequently. **Include a git message which describes what you have done**.

**If you do not push your code sufficiently frequently, or provide comments describing your work, you may lose points.**

Programming Assignment materials and resources
-------------------------------------------

You will download the skeleton code for the assignment from the CCIS GitHub  server, github. khoury.northeastern.edu, using the 'git clone' command:
```
 git clone https://github.khoury.northeastern.edu/cs5600-f23/team-nn-lab2
```
where *nn* is your team number. Periodically you will commit checkpoints of your work into your local repository using 'git commit':
```
    git commit -a -m 'message describing the checkin'
```
(or enter a message in the text editor) and push the commits to the central repository:
```
	git push
```
You should be able to complete this homework on any 64-bit Linux system; we are testing it on both the Intel and ARM class VMs.

Repository Contents
-----------------

The repository you clone contains the following files:

* `qthread.c` – this is the file you will be implementing. It has some comments describing the functions and types you have to implement.
* `qthread.h` – analogous to pthread.h, this defines the types used by qthread applications. 
* `test.c` – this is where your test code should go. (well, you can add additional files and update the makefile if you do)
* `Makefile` – this is set up to build the `test` executable. Compile your code by typing ‘make’. 

Deliverables
----------
The following files from your repository will be examined, tested, and graded:
* `qthread.c`, `qthread.h`
* `test.c`

Assignment description
--------------------

You will need to implement all the functions in the "qthreads" interface described below. To do this you'll almost certainly want to use the following design elements:

* a per-thread structure (let's call it `struct qthread`), holding the following information:
  * next pointer
  * thread context
  * timing information (see `qthread_sleep`)

* a FIFO queue of thread structures (let's call it `struct qthread_list`) for the active list and for lists of threads waiting on various things. (with a head and tail pointer, a `push_back` function that adds to the tail, a `pop_front` function that pops from the head, and an `empty` function to see if it's empty or not.

* the scheduling pattern described in lecture:
  * `struct qthread *current` - points to currently executing thread
  * `struct qthread_list active` - list of threads that are ready to run (i.e. not sleeping)

### Typedefs (provided in the qthread.h file)

`f_void_t` is a pointer to a function taking one `void*` argument and returning `void*`. You'll need this to cast a function pointer to pass it to `makecontext`. 
```
	typedef void * (*f_1arg_t)(void*);
```

I've provided the following (empty) structures and typedefs:
-  `struct qthread`, `qthread_t` - your qthread structure
- `struct qthread_mutex`, `qthread_mutex_t` - mutex
- `struct qthread_cond`, `qthread_cont_t` - condition variable

(note - there are several schools of thought on how to handle C structures. This approach - using a typedef instead of having to say "struct qthread" over and over - is a fairly common one in my experience.

### Functions (you implement)

Note that except for the first function, these are basically the POSIX thread functions with simplified interfaces. First the basic thread functions:

* `void qthread_init(void)` - this initializes your thread implementation
* `qthread_t qthread_create(f_1arg_t f, void *arg1)` - create a thread which executes `f(arg1)`.
* `void qthread_exit(void *val)` - exit (i.e. switch to another thread and never return) and save `val` as return value (see `qthread_join`)
* `void qthread_yield(void)` - put self on tail of active list and switch to the next active thread
* `void *qthread_join(qthread_t thread)` - wait for a thread to exit and get its return value. A thread can only be "joined" by one other thread. (i.e. it's OK to crash if two threads call join on the same thread)

Then mutexes and condition variables:

* `qthread_mutex_t *qthread_mutex_create(void)` 
* `void qthread_mutex_lock(qthread_mutex_t *m)`
* `void qthread_mutex_unlock(qthread_mutex_t *m)`
* `void qthread_mutex_destroy(qthread_mutex_t *m)`

* `qthread_cond_t *qthread_cond_create(void)`
* `void qthread_cond_wait(qthread_cond_t *c, qthread_mutex_t *m)`
* `void qthread_cond_signal(qthread_cond_t *c)`
* `void qthread_cond_broadcast(qthread_cond_t *c)`
* `void qthread_cond_destroy(qthread_cond_t *c)`

And finally a sleep function, with the same interface as the Unix `usleep` function:

* `void qthread_usleep(long int microsecs)`

## How to implement it

### thread control block 

Your thread control block (`struct qthread`) will need the following fields:
- `ucontext_t` structure, for context switching (see below)
- linked list pointer
- boolean flag indicating whether `qthread_exit` has been called
- return value (for `qthread_join`)
- pointer to thread waiting in `qthread_join`
- pointer to the stack you allocated, so you can free it in `qthread_join`

(note that although `qthread_exit` takes a thread out of the active queue, its resources aren't freed until another thread calls `qthread_exit`.


### Context switching

We'll use the `makecontext`, and `swapcontext` functions to switch between threads. (see "man makecontext").
In particular, what they do is:
- `makecontext(&ctx, f, 2, arg1, arg2)` - sets up a context with a new stack so that execution will start with `f(arg1,arg2)`
- `swapcontext(&ctx1,&ctx2)` - context switch between threads,  saving current stack in `ctx1` and switching to stack in `ctx2`

Before we call `makecontext` we'll have to set up a new stack in the context. Assuming your data structure is zeroed out - either because you allocated it with `calloc` or because you did it manually - you can do the following:
```
#define STACK_SIZE 64*1024
    void *stack = malloc(STACK_SIZE);
    memset(stack, 0xA5, STACK_SIZE);
    th->ctx.uc_stack = (stack_t) {
        .ss_sp = stack,
        .ss_flags = 0,
        .ss_size = STACK_SIZE
    };
```

Note that I'm initializing the stack to a "flag" value to make errors more visible - if I accidentally use an uninitialized stack value as a pointer, it's guaranteed to crash and will have a distinctive value: 0xA5A5A5A5A5A5A5A5

You'll need a "wrapper" function for the function passed to `qthread_create`, because - like pthreads - the qthreads interface allows the thread function to return, so you'll have to capture that value and pass it to `qthread_exit`. If the user creates a thread with function `f` and argument `arg1`, you'll invoke `qthread_wrapper(f, arg1)`.

Now you can set up the context to call your `qthread_wrapper` function:
```
qthread_t *qthread_create(void*(*f)(void*), void *arg)
{
   /* ... */
   makecontext(&th->ctx, (f_void_t)qthread_wrapper, 2, f, arg);
```
(yes, this is a bit weird. It tells `makecontext` to start the thread by calling `qthread_wrapper` with 2 arguments: `f` and `arg`. And no, there's absolutely no typechecking here)

Finally you can switch between two threads with `swapcontext(&ctx1,&ctx2)`,  which does the actual context switching. (i.e. saving/restoring registers and switching the stack pointer)

### `qthread_init`

`qthread_init()` creates a thread structure for the original OS-provided thread and puts it on `current`, so you can switch away from the original thread to another one and hopefully switch back again eventually. (this function has no equivalent in pthreads, although some user-space thread packages like GNU Pth have a similar init function)

Note that you don't have to use `makecontext` to initialize the thread context inside this control block, since the thread is already running. Instead, `swapcontext` will store the current state there, at the point when it context switches.

### exit/join

`qthread_exit` and `qthread_join` - You're going to have to stash the return value (from 'exit') in the thread structure so that 'join' can retrieve it. If 'join' is called first, it has to sleep until the other thread calls 'exit', which wakes it up. This means you're going to need the following fields in your thread structure:

* return value (i.e. the argument to 'exit')
* flag indicating whether 'exit' was already called
* pointer to a thread waiting in 'join' (or NULL if there isn't one)

### Current/Active

I'm assuming that you use the thread scheduler structure mentioned in lecture:
```
 +-------+     	  +--------------+
 |current|------> | thread block |--> null
 +-------+     	  +--------------+

 +--------+
 |active: |       +--------+   +--------+   +--------+
 |  front |-----> | thread |-->| thread |-->| thread |--> null
 |        |       +--------+   +--------+   +--------+
 |        |                                 ^
 |  back  |--------------------------------/
 +--------+
```

Here are the possible ways that we can manipulate *current* and *active* for thread scheduling:

1. give another thread a turn (but don't sleep):  
    append *current* to tail of *active*  
    *current* = [remove head of *active*]  
    switch to new *current*  
  
2. go to sleep:  
    [somewhere] = current  
    *current* = [remove head of *active*]  
    switch to new *current*  
  
3. wake another thread up (without switching)
    *th* = [somewhere]
    append *th* to tail of *active*

A blocking wait (e.g. on a condition variable or locked mutex) is implemented by putting a pointer to the current thread somewhere safe (but not on the active list), and then switching to the next thread in *active*. Similarly, waking another thread up means taking a pointer to that thread from somewhere and putting it back on the active list.

Typically the two steps:

* *current* = [remove head of *active*]
* switch to new *current*

are combined into a function called `schedule`, which picks the next thread and switches to it. (factoring this out will be very useful when you write `qthread_usleep`)

Finally, note that qthreads doesn't have to be huge and complex - my implementation is a bit under 250 lines of code, which includes the 97 lines of skeleton code you've been given.
