#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>

#include <pthread/rwlock_fcfs.h>

static const pthread_mutex_t initial_mutex_constant = 
    PTHREAD_MUTEX_INITIALIZER
    ;

pthread_rwlock_fcfs_t * mylock;

int stop = 0;
int num_active_threads = 0;
pthread_mutex_t num_active_mutex;

struct context_struct
{
    int index;
};

typedef struct context_struct context_t;

void * reader_thread(void * void_context)
{
    char id[100];
    context_t * context;

    context = (context_t *)void_context;

    sprintf(id, "Reader %i", context->index);

    free(void_context);

    pthread_mutex_lock(&num_active_mutex);
    num_active_threads++;
    pthread_mutex_unlock(&num_active_mutex);

    while (!stop)
    {
        int which = rand()%3;
        if (which == 0)
        {
            pthread_rwlock_fcfs_gain_read(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

            usleep(rand()%1000000);

            pthread_rwlock_fcfs_release(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

            usleep(rand()%1000000);
        }
        else if (which == 1)
        {
            struct timeval now;
            struct timespec timeout;
            struct timezone tz;
            int ret;
            
            gettimeofday(&now, &tz);
            timeout.tv_nsec = (now.tv_usec + (rand()%1000000));
            if (timeout.tv_nsec > 1000000)
            {
                timeout.tv_sec += timeout.tv_nsec / 1000000;
                timeout.tv_nsec %= 1000000;
            }
            
            timeout.tv_nsec *= 1000;
            ret = pthread_rwlock_fcfs_timed_gain_read(
                mylock,
                &timeout,
                NULL,
                NULL
                PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS
                );

            usleep(rand()%1000000);

            if (ret == 0)
            {
                pthread_rwlock_fcfs_release(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
            } 

            usleep(rand()%1000000);
            fflush(stdout);
        }
        else if (which == 2)
        {
            int ret;
            ret = pthread_rwlock_fcfs_try_gain_read(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

            usleep(rand()%1000000);            

            if (ret == 0)
            {
                pthread_rwlock_fcfs_release(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
            }

            usleep(rand()%1000000);
            fflush(stdout);
        }
    }

    pthread_mutex_lock(&num_active_mutex);
    num_active_threads--;
    pthread_mutex_unlock(&num_active_mutex);  

    return NULL;
}

void * writer_thread(void * void_context)
{
    char id[100];
    context_t * context;

    context = (context_t *)void_context;

    sprintf(id, "Writer %i", context->index);

    free(void_context);

    pthread_mutex_lock(&num_active_mutex);
    num_active_threads++;
    pthread_mutex_unlock(&num_active_mutex);

    while (!stop)
    {
        int which = rand()%3;
        if (which == 0)
        {
            pthread_rwlock_fcfs_gain_write(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

            usleep(rand()%1000000);

            pthread_rwlock_fcfs_release(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

            usleep(rand()%1000000);
        }
        else if (which == 1)
        {
            struct timeval now;
            struct timespec timeout;
            struct timezone tz;
            int ret;
            
            gettimeofday(&now, &tz);
            timeout.tv_nsec = (now.tv_usec + (rand()%1000000));
            if (timeout.tv_nsec > 1000000)
            {
                timeout.tv_sec += timeout.tv_nsec / 1000000;
                timeout.tv_nsec %= 1000000;
            }
            
            timeout.tv_nsec *= 1000;
            ret = pthread_rwlock_fcfs_timed_gain_write(
                mylock,
                &timeout,
                NULL,
                NULL
                PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS
                );

            usleep(rand()%1000000);

            if (ret == 0)
            {
                pthread_rwlock_fcfs_release(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
            } 

            usleep(rand()%1000000);
            fflush(stdout);
        }
        else if (which == 2)
        {
            int ret;
            ret = pthread_rwlock_fcfs_try_gain_write(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

            usleep(rand()%1000000);            

            if (ret == 0)
            {
                pthread_rwlock_fcfs_release(mylock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
            }

            usleep(rand()%1000000);
            fflush(stdout);
        }
    }

    pthread_mutex_lock(&num_active_mutex);
    num_active_threads--;
    pthread_mutex_unlock(&num_active_mutex);  

    return NULL;
}

int main(int argc, char * argv[])
{
    context_t * context;
    pthread_t * readers;
    pthread_t * writers;
    int check;

    int a;
    int arg;
    int NUM_READERS = 5;
    int NUM_WRITERS = 2;
    int timeout = -1;

    for(arg=1;arg<argc;arg++)
    {
        if (!strcmp(argv[arg], "--num-readers"))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(stderr, "--num-readers accepts an argument!\n");
                exit(-1);
            }
            NUM_READERS = atoi(argv[arg]);
        }
        else if (!strcmp(argv[arg], "--num-writers"))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(stderr, "--num-writers accepts an argument!\n");
                exit(-1);
            }
            NUM_WRITERS = atoi(argv[arg]);
        }
        else if (!strcmp(argv[arg], "--timeout"))
        {
            arg++;
            if (arg == argc)
            {
                fprintf(stderr, "--num-writers accepts an argument!\n");
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

    num_active_mutex = initial_mutex_constant;
    pthread_mutex_init(&num_active_mutex, NULL);

    readers = malloc(sizeof(readers[0])*NUM_READERS);
    writers = malloc(sizeof(writers[0])*NUM_WRITERS);

    mylock = pthread_rwlock_fcfs_alloc();
    for(a=0;a<NUM_READERS;a++)
    {
        context = malloc(sizeof(*context));
        context->index = a;
        check = pthread_create(
            &readers[a],
            NULL,
            reader_thread,
            context
            );
        
        if (check != 0)
        {
            fprintf(stderr, "Could not create Reader #%i!\n", a);
            exit(-1);
        }
    }

    for(a=0;a<NUM_WRITERS;a++)
    {
        context = malloc(sizeof(*context));
        context->index = a;
        check = pthread_create(
            &writers[a],
            NULL,
            writer_thread,
            context
            );
        
        if (check != 0)
        {
            fprintf(stderr, "Could not create Writer #%i!\n", a);
            exit(-1);
        }
    }

    if (timeout < 0)
    {
        while(1)
        {
            sleep(1);
        }
    }
    else
    {
        int local_num_active = 1;
        sleep(timeout);
        stop = 1;
        
        while (local_num_active)
        {
            usleep(500);
            pthread_mutex_lock(&num_active_mutex);
            local_num_active = num_active_threads;
            pthread_mutex_unlock(&num_active_mutex);
        }
    }

    pthread_rwlock_fcfs_destroy(mylock);

    for(a=0;a<NUM_READERS;a++)
    {
        pthread_join(readers[a], NULL);
    }
    for(a=0;a<NUM_WRITERS;a++)
    {
        pthread_join(writers[a], NULL);
    }
    

    free(readers);
    free(writers);

    return 0;
}

