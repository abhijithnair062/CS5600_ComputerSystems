/*
 * file:        test.c
 * description: tests for Lab 2
 *  Peter Desnoyers, CS 5600, Fall 2023
 *  
 * to compile and run you will need to install libcheck:
 *   sudo apt install check
 *
 * libcheck forks a subprocess for each test, so that it can run
 * to completion even if your tests segfault. This makes debugging
 * very difficult; to turn it off do:
 *   CK_FORK=no gdb ./test
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <unistd.h>

#include "qthread.h"
#define CK_DEFAULT_TIMEOUT 10
/* see the libcheck documentation for more details:
 *   https://libcheck.github.io/check/
 * API documentation here: 
 *   https://libcheck.github.io/check/doc/doxygen/html/check_8h.html
 */
qthread_list activeList;
qthread_mutex_t* m;
qthread_cond_t *c1,*c2,*c3;
int flag;

void setupThreadList(){
    activeList.head = NULL;
    activeList.tail = NULL;
}

void printloopFunction(void *msg){
    for(int i=0; i < 10; i++){
        printf("%s : %d\n",(char*)msg,i);
        flag+=1;
    }
}

void printloopFunctionWithYield(void *msg){
    for(int i=0; i < 10; i++){
        printf("%s : %d\n",(char*)msg,i);
        flag+=1;
        usleep(300000);
        qthread_yield();
    }
}

void printloopFunctionForSimpleJoin(void *msg){
    for(int i=0; i < 10; i++){
        printf("%s : %d\n",(char*)msg,i);
        flag+=1;
        usleep(300000);
        qthread_yield();
    }
    qthread_exit(msg);
}

void printloopFunctionForComplexJoin(void *joinee){
    if(joinee != NULL){
        qthread_join(joinee);
    }
    for(int i=0; i < 3; i++){
        printf("%d\n",i);
        flag+=1;
        usleep(300000);
        qthread_yield();
    }
    qthread_exit("Thread completed");
}

void printloopFunctionWithSleep(void *msg){
    int i;
    for( i=0; i < 10; i++){
        flag+=1;
        printf("%s : %d\n",(char*)msg,i);
        qthread_usleep(3000000);
    }
}

void printloopMultipleThreadsWithSleep(void *msg){
    int i;
    for( i=0; i < 10; i++){
        flag+=1;
        printf("%s : %d\n",(char*)msg,i);
        qthread_usleep(3000000);
    }
}

void* printloopFuncSleepAndJoin(void *msg){
    for(int i=0; i < 10; i++){
        printf("%s : %d\n",(char*)msg,i);
        qthread_usleep(3000000);
    }
    return msg;
}

void* exitAndJoinTest(void *msg){
    return msg;
}

void *single_thread_mutex(void *msg)
{
    flag += 1;
    qthread_mutex_lock(m);
    flag += 2;
    qthread_yield();
    qthread_yield();
    qthread_mutex_unlock(m);
    return msg;
}

void multi_thread_mutex(void *msg)
{
    for (int i = 0; i < 10; i++)
    {
        qthread_mutex_lock(m);
        flag++;
        printf("%s : %d\n",(char*) msg, flag);
        qthread_mutex_unlock(m);
    }
}

void multi_thread_mutex_yield(void *msg)
{
    for (int i = 0; i < 10; i++)
    {
        qthread_mutex_lock(m);
        flag++;
        printf("%s : %d\n",(char*) msg, flag);
        qthread_mutex_unlock(m);
        qthread_yield();
    }
}

void multi_thread_mutex_sleep(void *msg)
{
    for (int i = 0; i < 10; i++)
    {
        qthread_mutex_lock(m);
        flag++;
        printf("%s : %d\n",(char*) msg, flag);
        qthread_usleep(3000000);
        qthread_mutex_unlock(m);
    }
}

void condVarFun1(void *msg)
{
    int i;
    qthread_usleep(10000);
    qthread_mutex_lock(m);
    for (i = 0; i < 10; i++)
    {
        if(i > 0){
            qthread_cond_wait(c1,m);
        }
        printf("%s : %d\n",(char*) msg, i);
        qthread_usleep(200000);
        qthread_cond_signal(c2);
    }
    qthread_mutex_unlock(m);
}

void condVarFun2(void *msg)
{
    int i;
    qthread_usleep(10000);
    qthread_mutex_lock(m);
    for (i = 0; i < 10; i+=2)
    {
        qthread_cond_wait(c2,m);
        printf("%s : %d\n",(char*) msg, i);
        qthread_usleep(200000);
        qthread_cond_signal(c3);

        qthread_cond_wait(c2,m);
        printf("%s : %d\n",(char*) msg, i);
        qthread_usleep(200000);
        qthread_cond_signal(c1);
    }
    qthread_mutex_unlock(m);
}

void condVarFun3(void *msg)
{
    int i;
    qthread_usleep(10000);
    qthread_mutex_lock(m);
    for (i = 0; i < 10; i++)
    {
        qthread_cond_wait(c3,m);
        printf("%s : %d\n",(char*) msg, i);
        qthread_usleep(200000);
        qthread_cond_signal(c2);
    }
    qthread_mutex_unlock(m);
}

void simpleCondVar1(void* msg){
    qthread_mutex_lock(m);
    for(int i = 0; i < 10; i++){
        if(i == 0){
            qthread_cond_wait(c1,m);
        }
        printf("%s : %d\n",(char*)msg,i);
        flag+=1;
    }
    qthread_mutex_unlock(m);
}

void simpleCondVar2(void* msg){
    qthread_mutex_lock(m);
    qthread_cond_signal(c1);
    qthread_mutex_unlock(m);
}
void secondCondVar1(void* msg){
    qthread_usleep(10000);
    qthread_mutex_lock(m);
    for(int i = 0; i < 10; i++){
        if(i == 0){
            qthread_cond_wait(c1,m);
        }
        printf("%s : %d\n",(char*)msg,i);
        flag +=1;
        qthread_usleep(10000);
    }
    qthread_mutex_unlock(m);
}
void secondCondVar2(void* msg){
    qthread_usleep(10000);
    qthread_mutex_lock(m);
    qthread_usleep(10000);
    qthread_cond_signal(c1);
    qthread_mutex_unlock(m);
}
void thirdCondVar1(void* msg){
    int i;
    qthread_usleep(10000);
    qthread_mutex_lock(m);
    for(i = 0; i < 10; i++){
        if(i == 0){
            qthread_cond_wait(c1,m);
        }
        printf("%s : %d\n",(char*)msg,i);
        flag+=1;
    }
    qthread_mutex_unlock(m);
}
void thirdCondVar2(void* msg){
    int i;
    qthread_usleep(10000);
    qthread_mutex_lock(m);
    for(i= 0 ;i < 10 ; i+=2){
        if(i == 0){
            qthread_cond_wait(c1,m);
        }
        printf("%s : %d\n",(char*)msg,i);
        flag+=1;
    }
    qthread_usleep(10000);
    qthread_cond_signal(c1);
    qthread_mutex_unlock(m);
}
void thirdCondVar3(void* msg){
    qthread_usleep(10000);
    qthread_cond_broadcast(c1);
}

void thirdCondVar4(void* msg){
    qthread_usleep(10000);
    qthread_cond_signal(c1);
    qthread_usleep(10000);
    qthread_cond_signal(c1);
}
void fourthCondVar1(void* msg){
    qthread_mutex_lock(m);
    for(int i = 0; i < 10; i++){
        if(i == 0){
            qthread_cond_wait(c1,m);
        }
        ck_assert(m->locked);
        printf("%s : %d\n",(char*)msg,i);
        flag+=1;
    }
    qthread_mutex_unlock(m);
}

void fourthCondVar2(void* msg){
    ck_assert(!m->locked);
    qthread_mutex_lock(m);
    qthread_cond_signal(c1);
    qthread_mutex_unlock(m);
}
void yieldHelper(){
    for(int i = 0 ;i < 30 ;i++){
        qthread_yield();
        usleep(30000);
    }
}
START_TEST(test_1){
        printf("Test1\n");
        ck_assert_int_eq(1, 1);
        ck_assert(1 == 1);
        ck_assert_int_ge(5, 1);
        ck_assert_int_le(2, 3);     /* also _gt, _lt, _ne */
        ck_assert_msg(1==1, "failed: 1 != 1");
        if (1 != 1)
        ck_abort_msg("impossible result");

                /* if you're testing the sleep function, you might want to
                 * use ck_assert_float_eq_tol, which compares two floating point
                 * numbers for equality within a specified tolerance.
                 *
                 *  double t1 = get_usecs();
                 *  qthread_usleep(100000);
                 *  double t2 = get_usecs();
                 *  ck_assert_float_eq_tol(t2-t1, 100000, 100); // += 100 usec
                 */
}
START_TEST(test_2){
        //Test isEmpty on empty active list
        printf("Test2\n");
        setupThreadList();
        ck_assert(isEmpty(&activeList));

}

START_TEST(test_3){
        //Test pop before push back
        printf("Test3\n");
        setupThreadList();
        qthread_t* result = pop_front(&activeList);
        ck_assert_ptr_null(result);
}

START_TEST(test_4){
        //Test push_back
        printf("Test4\n");
        setupThreadList();
        qthread_t* thread = calloc(sizeof(*thread),1);
        push_back(&activeList,thread);
        ck_assert(!isEmpty(&activeList));
        free(thread);
}

START_TEST(test_5){
        //Test push_back multiple times
        printf("Test5\n");
        setupThreadList();
        qthread_t* thread1 = calloc(sizeof(*thread1),1);
        qthread_t* thread2 = calloc(sizeof(*thread2),1);
        push_back(&activeList,thread1);
        push_back(&activeList,thread2);
        ck_assert(!isEmpty(&activeList));
        free(thread1);
        free(thread2);
}


START_TEST(test_7){
        //Test single create thread
        printf("Test7\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunction;
        qthread_t *thread1 = qthread_create(f,"thread 1");
        qthread_yield();
        free(thread1);
        ck_assert_int_eq(flag,10);
        printf("all done\n");
}

START_TEST(test_8){
        //Test multiple create threads
        printf("Test8\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunction;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_t* thread2 = qthread_create(f,"thread 2");
        qthread_yield();
        free(thread1);
        free(thread2);
        ck_assert_int_eq(flag,20);
        printf("all done\n");
}

START_TEST(test_9){
        //Test multiple threads with yield
        printf("Test9\n");
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionWithYield;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_t* thread2 = qthread_create(f,"thread 2");
        qthread_t* thread3 = qthread_create(f,"thread 3");
        qthread_yield();
        free(thread1);
        free(thread2);
        free(thread3);
        printf("all done\n");
}

START_TEST(test_10){
        //Test simple join
        printf("Test10\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionForSimpleJoin;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_t* thread2 = qthread_create(f,"thread 2");
        qthread_t* thread3 = qthread_create(f,"thread 3");

        void *val;
        val = qthread_join(thread1);
        printf("Join 1 : %s\n",(char *)val);

        val = qthread_join(thread2);
        printf("Join 2 : %s\n",(char *)val);

        val = qthread_join(thread3);
        printf("Join 3 : %s\n",(char *)val);

        ck_assert_int_eq(flag,30);

        printf("all done\n");

}

START_TEST(test_11){
        //Test thread1 calls exit after thread 2 calls join
        printf("Test11\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionForComplexJoin;
        qthread_t* thread1 = qthread_create(f,NULL);
        qthread_t* thread2 = qthread_create(f,thread1);
        qthread_t* thread3 = qthread_create(f,thread2);

        yieldHelper();
        free(thread3);
        ck_assert_int_eq(flag,9);
        printf("All done\n");

}

START_TEST(test_12){
        //Test thread 1 has already called exit before thread 2 calls join
        printf("Test12\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionForSimpleJoin;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_t* thread2 = qthread_create(f,"thread 2");
        qthread_t* thread3 = qthread_create(f,"thread 3");

        void *val;
        val = qthread_join(thread1);
        printf("Join 1 : %s\n",(char *)val);

        val = qthread_join(thread2);
        printf("Join 2 : %s\n",(char *)val);

        val = qthread_join(thread3);
        printf("Join 3 : %s\n",(char *)val);
        ck_assert_int_eq(flag,30);
        printf("all done\n");

}

START_TEST(test_13){
        //Test multiple threads with yield multiple times
        printf("Test13\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionWithYield;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_t* thread2 = qthread_create(f,"thread 2");
        qthread_t* thread3 = qthread_create(f,"thread 3");
        yieldHelper();
        ck_assert_int_eq(flag,30);
        free(thread1);
        free(thread2);
        free(thread3);
        printf("All done\n");
}


START_TEST(test_14){
        //Test single thread with qthread_sleep
        printf("Test14\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionWithSleep;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_yield();
        ck_assert_int_eq(flag,10);
        free(thread1);
        printf("All done\n");
}

START_TEST(test_15){
        //Yield before join
        printf("Test15\n");
        void* (*f)(void*) = (void *(*)(void *)) exitAndJoinTest;
        void* msg = "thread 1";
        qthread_t* thread1 = qthread_create(f,msg);
        qthread_yield();
        void* val = qthread_join(thread1);
        ck_assert_str_eq((char*)msg,(char*)val);
        printf("All done\n");
}

START_TEST(test_16){
        //Yield after Join
        printf("Test16\n");
        void* (*f)(void*) = (void *(*)(void *)) exitAndJoinTest;
        void* msg = "thread 1";
        qthread_t* thread1 = qthread_create(f,msg);
        void* val = qthread_join(thread1);
        qthread_yield();
        ck_assert_str_eq((char*)msg,(char*)val);
        printf("All done\n");
}

START_TEST(test_17){
        //This test checks if the basic functionalities works 1
        printf("Test17\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionWithYield;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        yieldHelper();
        free(thread1);
        ck_assert_int_eq(flag,10);
        printf("all done\n");
}

START_TEST(test_18){
        //This test which calls qthread_yield()
        printf("Test18\n");
        yieldHelper();
        printf("all done\n");
}

START_TEST(test_19){
        //This test checks if the basic functionalities works 2
        printf("Test19\n");
        flag = 0;
        void* (*f)(void*) = (void *(*)(void *)) printloopFunctionWithYield;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_t* thread2 = qthread_create(f,"thread 2");
        qthread_t* thread3 = qthread_create(f,"thread 3");
        yieldHelper();
        free(thread1);
        free(thread2);
        free(thread3);
        ck_assert_int_eq(flag,30);
        printf("all done\n");
}

START_TEST(test_20){
        //Test thread with join and sleep -> yield after join
        printf("Test20\n");
        void* (*f)(void*) = (void *(*)(void *)) printloopFuncSleepAndJoin;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        void* val = qthread_join(thread1);
        qthread_yield();
        ck_assert_str_eq("thread 1",(char*)val);
        printf("All done\n");
}

START_TEST(test_21){
        //Test thread with join and sleep -> yield before join
        printf("Test21\n");
        void* (*f)(void*) = (void *(*)(void *)) printloopFuncSleepAndJoin;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_yield();
        void* val = qthread_join(thread1);
        ck_assert_str_eq("thread 1",(char*)val);
        printf("All done\n");
}

START_TEST(test_22){
        //Weird combinations
        printf("Test22\n");
        qthread_yield();
        void* (*f)(void*) = (void *(*)(void *)) printloopFuncSleepAndJoin;
        qthread_t* thread1 = qthread_create(f,"thread 1");
        qthread_yield();
        qthread_yield();
        qthread_yield();
        void* val = qthread_join(thread1);
        qthread_yield();
        qthread_yield();
        ck_assert_str_eq("thread 1",(char*)val);
        printf("All done\n");
}

START_TEST(test_23){
        //Mutex testing 1
        printf("Test23\n");
        flag = 0;
        m = qthread_mutex_create();
        qthread_mutex_lock(m);
        ck_assert(m->locked==1);
        qthread_mutex_unlock(m);
        ck_assert(m->locked==0);
        qthread_mutex_destroy(m);
        printf("All done\n");
}

START_TEST(test_24){
        //Single thread locking and unlocking a mutex
        printf("Test24\n");
        flag = 0;
        m = qthread_mutex_create();

        qthread_mutex_lock(m);
        void *(*f)(void *) = (void *(*)(void *))single_thread_mutex;
        qthread_t *thread1 = qthread_create(f, "thread 1");
        qthread_yield();
        qthread_yield();
        ck_assert(flag == 1);
        qthread_mutex_unlock(m);
        qthread_join(thread1);
        ck_assert(flag == 3);
        qthread_mutex_destroy(m);
        printf("All done\n");
        }

START_TEST(test_25){
        //Three threads locking and unlocking a mutex 1
        printf("Test25\n");
        flag = 0;
        m = qthread_mutex_create();
        void *(*f)(void *) = (void *(*)(void *))multi_thread_mutex;
        qthread_t *thread1 = qthread_create(f, "thread 1");
        qthread_t *thread2 = qthread_create(f, "thread 2");
        qthread_t *thread3 = qthread_create(f, "thread 3");
        qthread_join(thread1);
        qthread_join(thread2);
        qthread_join(thread3);
        ck_assert(flag == 30);
        qthread_mutex_destroy(m);
        printf("All done\n");
}

START_TEST(test_26){
        //Three threads locking and unlocking a mutex 2
        printf("Test26\n");
        flag = 0;
        m = qthread_mutex_create();

        void *(*f)(void *) = (void *(*)(void *))multi_thread_mutex_yield;
        qthread_t *thread1 = qthread_create(f, "thread 1");
        qthread_t *thread2 = qthread_create(f, "thread 2");
        qthread_t *thread3 = qthread_create(f, "thread 3");

        qthread_join(thread1);
        qthread_join(thread2);
        qthread_join(thread3);
        ck_assert(flag == 30);
        qthread_mutex_destroy(m);
        printf("All done\n");
}

START_TEST(test_27){
    //Three threads locking and unlocking a mutex 3
        printf("Test27\n");

        flag = 0;
        m = qthread_mutex_create();

        void *(*f)(void *) = (void *(*)(void *))multi_thread_mutex_sleep;
        qthread_t *thread1 = qthread_create(f, "thread 1");
        qthread_t *thread2 = qthread_create(f, "thread 2");
        qthread_t *thread3 = qthread_create(f, "thread 3");

        qthread_join(thread1);
        qthread_join(thread2);
        qthread_join(thread3);
        ck_assert(flag == 30);
        qthread_mutex_destroy(m);
        printf("All done\n");
}

START_TEST(test_28){
        //Three threads Condition Variable - Same test as in the video provided by Professor
        printf("Test28\n");

        flag = 0;
        m = qthread_mutex_create();

        c1 = qthread_cond_create();
        c2 = qthread_cond_create();
        c3 = qthread_cond_create();

        void *(*f1)(void *) = (void *(*)(void *))condVarFun1;
        void *(*f2)(void *) = (void *(*)(void *))condVarFun2;
        void *(*f3)(void *) = (void *(*)(void *))condVarFun3;
        qthread_t *thread1 = qthread_create(f1, "thread 1");
        qthread_t *thread2 = qthread_create(f2, "thread 2");
        qthread_t *thread3 = qthread_create(f3, "thread 3");

        qthread_join(thread3);
        qthread_join(thread2);
        qthread_join(thread1);

        qthread_mutex_destroy(m);
        qthread_cond_destroy(c1);
        qthread_cond_destroy(c2);
        qthread_cond_destroy(c3);

        printf("All done\n");
}

START_TEST(test_29){
        //One thread can wait and be woken up with signal
        printf("Test29\n");
        flag = 0;
        m = qthread_mutex_create();
        c1 = qthread_cond_create();
        void *(*f1)(void *) = (void *(*)(void *))simpleCondVar1;
        void *(*f2)(void *) = (void *(*)(void *))simpleCondVar2;

        qthread_t *thread1 = qthread_create(f1, "thread 1");
        qthread_t *thread2 = qthread_create(f2, "thread 2");

        qthread_join(thread1);
        qthread_join(thread2);
        qthread_mutex_destroy(m);
        qthread_cond_destroy(c1);
        ck_assert_int_eq(flag,10);

        printf("All done\n");
}

START_TEST(test_30){
        //Two threads can wait and be woken up with signal
        printf("Test30\n");
        flag = 0;
        m = qthread_mutex_create();
        c1 = qthread_cond_create();
        void *(*f1)(void *) = (void *(*)(void *))secondCondVar1;
        void *(*f2)(void *) = (void *(*)(void *))secondCondVar2;

        qthread_t *thread1 = qthread_create(f1, "thread 1");
        qthread_t *thread2 = qthread_create(f2, "thread 2");

        qthread_join(thread1);
        qthread_join(thread2);
        yieldHelper();
        qthread_mutex_destroy(m);
        qthread_cond_destroy(c1);
        printf("All done!\n");
}

START_TEST(test_31){
    //Two threads can wait and be woken up with broadcast
    //This test also covers qhtread_sleep() condition where other thread is waiting on condvar
    printf("Test31\n");
    flag = 0;
    m = qthread_mutex_create();
    c1 = qthread_cond_create();
    void *(*f1)(void *) = (void *(*)(void *))thirdCondVar1;
    void *(*f2)(void *) = (void *(*)(void *))thirdCondVar2;
    void *(*f3)(void *) = (void *(*)(void *))thirdCondVar3;

    qthread_t *thread1 = qthread_create(f1, "thread 1");
    qthread_t *thread2 = qthread_create(f2, "thread 2");
    qthread_t *thread3 = qthread_create(f3, "thread 3");

    qthread_join(thread3);
    qthread_join(thread2);
    qthread_join(thread1);
    yieldHelper();
    ck_assert_int_eq(flag,15);
    qthread_mutex_destroy(m);
    qthread_cond_destroy(c1);
    printf("Flag value : %d\n",flag);
    printf("All done!\n");
}
START_TEST(test_32){
    //Two threads can wait and be woken up with two signals
    printf("Test33\n");
    flag = 0;
    m = qthread_mutex_create();
    c1 = qthread_cond_create();
    void *(*f1)(void *) = (void *(*)(void *))thirdCondVar1;
    void *(*f2)(void *) = (void *(*)(void *))thirdCondVar2;
    void *(*f3)(void *) = (void *(*)(void *))thirdCondVar4;

    qthread_t *thread1 = qthread_create(f1, "thread 1");
    qthread_t *thread2 = qthread_create(f2, "thread 2");
    qthread_t *thread3 = qthread_create(f3, "thread 3");

    qthread_join(thread3);
    qthread_join(thread2);
    qthread_join(thread1);
    yieldHelper();
    ck_assert_int_eq(flag,15);
    qthread_mutex_destroy(m);
    qthread_cond_destroy(c1);
    printf("Flag value : %d\n",flag);
    printf("All done!\n");
}
START_TEST(test_33){
    //Wait unlocks the mutex
    printf("Test29\n");
    m = qthread_mutex_create();
    c1 = qthread_cond_create();
    void *(*f1)(void *) = (void *(*)(void *))fourthCondVar1;
    void *(*f2)(void *) = (void *(*)(void *))fourthCondVar2;

    qthread_t *thread1 = qthread_create(f1, "thread 1");
    qthread_t *thread2 = qthread_create(f2, "thread 2");

    qthread_join(thread1);
    qthread_join(thread2);
    qthread_mutex_destroy(m);
    qthread_cond_destroy(c1);

    printf("All done\n");
}

START_TEST(test_34){
    //Other threads run(calling yield) while one thread is sleeping in qthread_usleep
    printf("Test34\n");
    flag = 0;
    void* (*f)(void*) = (void *(*)(void *)) printloopMultipleThreadsWithSleep;
    qthread_t* thread1 = qthread_create(f,"thread 1");
    qthread_t* thread2 = qthread_create(f,"thread 2");
    qthread_yield();
    ck_assert_int_eq(flag,20);
    free(thread1);
    free(thread2);
    printf("All done\n");
}


Suite *create_suite(void)
{
    Suite *s = suite_create("qthreads");
    TCase *tc = NULL;

    /* you can add all your tests to a single "test case"
     */
    tc = tcase_create("first test set");{
        tcase_add_test(tc, test_1);
        tcase_add_test(tc, test_2);
        tcase_add_test(tc, test_3);
        tcase_add_test(tc, test_4);
        tcase_add_test(tc, test_5);
        tcase_add_test(tc, test_7);
        tcase_add_test(tc, test_8);
        tcase_add_test(tc, test_9);
        tcase_add_test(tc, test_10);
        tcase_add_test(tc, test_11);
        tcase_add_test(tc, test_12);
        tcase_add_test(tc, test_13);
        tcase_add_test(tc, test_14);
        tcase_add_test(tc, test_15);
        tcase_add_test(tc, test_16);
        tcase_add_test(tc, test_17);
        tcase_add_test(tc, test_18);
        tcase_add_test(tc, test_19);
        tcase_add_test(tc, test_20);
        tcase_add_test(tc, test_21);
        tcase_add_test(tc, test_22);
/*************************MUTEX TESTS************************/
        tcase_add_test(tc, test_23);
        tcase_add_test(tc, test_24);
        tcase_add_test(tc, test_25);
        tcase_add_test(tc, test_26);
        tcase_add_test(tc, test_27);
        /***********************CV TEST******************/
        tcase_add_test(tc, test_28);
        tcase_add_test(tc, test_29);
        tcase_add_test(tc, test_30);
        tcase_add_test(tc, test_31);
        tcase_add_test(tc, test_32);
        tcase_add_test(tc, test_33);
        tcase_add_test(tc, test_34);
    }

    tcase_set_timeout(tc,180);
    suite_add_tcase(s, tc);

    /* or you can add some structure by adding more test cases:
     *  tc = tcase_create("another test set");
     *  tcase_add_test(tc, test_x);
     *   ...
     *  suite_add_tcase(s, tc);
     */

    return s;
}

int main(int argc, char **argv)
{
    qthread_init();
    Suite *s = create_suite();
    SRunner *sr = srunner_create(s);
//    srunner_set_fork_status(sr, CK_NOFORK);//TODO(ABHIJITH): Comment this line before shipping
    srunner_run_all(sr, CK_VERBOSE);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    printf("%d tests failed\n", n_failed);
}