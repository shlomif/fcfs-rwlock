/*
 * testtest.cpp
 *
 * this is test case for rwlock_fcfs
 *  Created on: Apr 12, 2009
 *      Author: luo
 */


#include <stdio.h>
#include <string.h>
#include <pthread/rwlock_fcfs.h>
#include <unistd.h>
#include <stdlib.h>

const int MAX_THREAD = 100;
int to_continue = 1;

pthread_rwlock_fcfs_t* lock;

void * thread_work(void* attr)
{
    int tid = (int)attr;
    char string[100];
    char id[100];

    sprintf(id, "TID=%d", tid);

    while(to_continue)
    {
        pthread_rwlock_fcfs_gain_read(lock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
        puts("in class A");
        sprintf(string, "tid: %d read locking", tid);
        puts(string);
        pthread_rwlock_fcfs_release(lock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
        sprintf(string, "tid: %d read release", tid);
        puts(string);
        pthread_rwlock_fcfs_gain_write(lock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
        sprintf(string, "tid: %d write locking", tid);
        puts(string);
        pthread_rwlock_fcfs_release(lock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
        sprintf(string, "tid: %d write release", tid);
        puts(string);
        sleep(1);
    }

    return NULL;
}

int main(int argc, char * argv[]) 
{
    pthread_t threads[MAX_THREAD];
    int rc;
    void* status;
    int t;
    int timeout = -1;
    int arg;

    lock = pthread_rwlock_fcfs_alloc();

    for(arg=1;arg<argc;arg++)
    {
        if (!strcmp(argv[arg], "--timeout"))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(stderr, "--timeout accepts an argument!\n");
                exit(-1);
            }
            timeout = atoi(argv[arg]);
        }
        else
        {
            fprintf(stderr, "Unknown option - \"%s\"!\n", argv[arg]);
            exit(-1);
        }
    }

    for (t = 0 ; t < MAX_THREAD ; t++)
    {
        rc = pthread_create(&threads[t], NULL, thread_work, (void*) t);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            return -1;
        }
    }

    if (timeout > 0)
    {
        sleep(timeout);
    }

    to_continue = 0;

    for (t = 0 ; t < MAX_THREAD ; t++)
    {
        rc = pthread_join(threads[t], &status);
        if (rc)
        {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            return -1;
        }
        printf("Main: completed join with thread %ld having a status of %ld\n",
                (long)t, (long)status);
    }

    return 0;
}
