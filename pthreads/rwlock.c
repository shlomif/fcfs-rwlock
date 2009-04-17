#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include <pthread/rwlock_fcfs.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static const pthread_mutex_t initial_mutex_constant = PTHREAD_MUTEX_INITIALIZER;
static const pthread_cond_t initial_cond_constant = PTHREAD_COND_INITIALIZER;

#ifdef PTHREAD_RWLOCK_FCFS_DEBUG
#if 0
static void my_debug_print(pthread_rwlock_fcfs_t * rwlock, char * id, char * msg)
{
    printf("%s - %s\n", id, msg);
    printf("    Status: rwlock.num_readers == %i\n", rwlock->num_readers);
    printf("    Status: rwlock.num_pending_readers == %i\n", rwlock->num_pending_readers);
    printf("    Status: rwlock.status == %i\n", rwlock->status);
    {
        int a;
        pthread_rwlock_fcfs_queue_item_t * q_item;
        pthread_rwlock_fcfs_item_t * data;

        q_item = rwlock->queue->head;
        a = 0;
        while(q_item != rwlock->queue->tail)
        {
            data = (pthread_rwlock_fcfs_item_t *)q_item->data;
            printf("    Status: Item %i: is_writer=%i\n", a, data->is_writer);
            printf("    Status: Item %i: num_threads=%i\n", a, data->num_threads);
            printf("    Status: Item %i: was_first_thr_accepted=%i\n", a, data->was_first_thr_accepted);

            q_item = q_item->next;
            a++;
        }
        if (q_item != NULL)
        {
            data = (pthread_rwlock_fcfs_item_t *)q_item->data;
            printf("    Status: Item %i: is_writer=%i\n", a, data->is_writer);
            printf("    Status: Item %i: num_threads=%i\n", a, data->num_threads);
            printf("    Status: Item %i: was_first_thr_accepted=%i\n", a, data->was_first_thr_accepted);
        }
    }
}
#else
#define my_debug_print(rwlock,id,msg) { printf("%s - %s\n", id, msg); }
#endif
#else
#define my_debug_print(rwlock, id, msg) { }
#endif

pthread_rwlock_fcfs_t * pthread_rwlock_fcfs_alloc(void)
{
    pthread_rwlock_fcfs_t * rwlock;

    rwlock = malloc(sizeof(pthread_rwlock_fcfs_t));

    rwlock->queue = pthread_rwlock_fcfs_queue_alloc();

    /* We don't have any readers now. */
    rwlock->num_readers = 0;
    rwlock->num_pending_readers = 0;

    /* Much less a writer */
    rwlock->status = PTHREAD_RWLOCK_FCFS_UNLOCKED;

    /* This RWLock is alive and kicking */
    rwlock->is_destroyed = 0;

    /* Initialize the mutex */
    rwlock->mutex = initial_mutex_constant;
    pthread_mutex_init(&(rwlock->mutex), NULL);

    return rwlock;
}

static int gain_write_generic(
    pthread_rwlock_fcfs_t * rwlock,
    int write_for_destroy,
    int wait_for_access,
    int is_timed,
    const struct timespec * abstime,
    int (*continue_callback)(void * context),
    void * context
    )
{
    /* If the rwlock is going to be destroyed - exit now.
     * An exception is if we are gaining write access to clear up all
     * existing threads.
     * */
    if (rwlock->is_destroyed && !write_for_destroy)
    {
        return -1;
    }

    /* If there aren't any readers or writers in the queue we
     * can gain access immidiately */
    if (rwlock->status != PTHREAD_RWLOCK_FCFS_UNLOCKED)
    {
        pthread_rwlock_fcfs_item_t * item;

        if (! wait_for_access)
        {
            return 1;
        }

        item = (pthread_rwlock_fcfs_item_t *)pthread_rwlock_fcfs_queue_peak_tail(rwlock->queue);
        if (
                /* The queue is not empty */
                (item != NULL) &&
                /* It is a writers' pack */
                item->is_writer &&
                /* The first thread was not yet accepted from it */
                (! item->was_first_thr_accepted)
           )
        {
            item->num_threads++;
        }
        else
        {

            /* Someone is using the RWLock. Let's enqueue myself */
            item = malloc(sizeof(pthread_rwlock_fcfs_item_t));
            /* We are a writer, so let's designate ourselves as such */
            item->is_writer = 1;

            /* We want to gain access to the lock */
            item->is_disabled = 0;

            /* We created a new pack in which there's one thread */
            item->num_threads = 1;

            /* This is a new pack, so no threads were accepted yet. */
            item->was_first_thr_accepted = 0;

            /* Initialize the condition variable */
            item->cond = initial_cond_constant;

            pthread_cond_init(&(item->cond), NULL);

            /* Let's put ourselves in the queue, so we will be release
             * when necessary. */
            pthread_rwlock_fcfs_queue_enqueue(rwlock->queue, item);

        }

        /* Wait upon the condition variable to gain access to the rwlock */
        if (! is_timed)
        {
            pthread_cond_wait(&(item->cond), &(rwlock->mutex));
        }
        else
        {
            int ret;

            /*
             * We try to get a timed wait on cond. If it times-out, we ask
             * the user if he wishes to continue.
             */
            do
            {
                ret = pthread_cond_timedwait(&(item->cond), &(rwlock->mutex), abstime);
            } while
                (
                    (ret == ETIMEDOUT) &&
                    (continue_callback != NULL) &&
                    continue_callback(context)
                );

            /* We failed for some reason (probably a time out - let's disable
             * this item, so it won't be processed.
             * */
            if (ret != 0)
            {
                item->num_threads--;
                return ret;
            }
        }

        /* accept_pending_items is doing it for us. */
#if 0
        /* Now we have a write access */
        rwlock->is_writer = 1;
#endif

        return 0;
    }
    else
    {
        /* Mark the lock as such that contains a writer */
        rwlock->status = PTHREAD_RWLOCK_FCFS_USED_BY_A_WRITER;

        return 0;
    }
}


int pthread_rwlock_fcfs_gain_write(pthread_rwlock_fcfs_t * rwlock PTHREAD_RWLOCK_FCFS_DEBUG_ARGS)
{
    int ret;

    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Want Write Lock!");

    ret = gain_write_generic(rwlock, 0, 1, 0, NULL, NULL, NULL);

    my_debug_print(rwlock, id, "Gained Write Lock!");

    pthread_mutex_unlock(&(rwlock->mutex));

    return ret;
}

static int gain_write_for_destroy(pthread_rwlock_fcfs_t * rwlock PTHREAD_RWLOCK_FCFS_DEBUG_ARGS)
{
    int ret;

    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Want Write-Destroy Lock!");

    ret = gain_write_generic(rwlock, 1, 1, 0, NULL, NULL, NULL);

    my_debug_print(rwlock, id, "Gained Write-Destroy Lock!");

    pthread_mutex_unlock(&(rwlock->mutex));

    return ret;
}

int pthread_rwlock_fcfs_try_gain_write(pthread_rwlock_fcfs_t * rwlock PTHREAD_RWLOCK_FCFS_DEBUG_ARGS)
{
    int ret;

    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Want Try Write Lock!");

    ret = gain_write_generic(rwlock, 0, 0, 0, NULL, NULL, NULL);

    my_debug_print(rwlock, id, 
            ((ret == 0)
             ? "Gained Tried Write Lock!"
             : "Failed Tried Write Lock!")
            );

    pthread_mutex_unlock(&(rwlock->mutex));

    return ret;
}


/* This macro dequeues and frees a queue element */
#define dequeue_and_destroy_item() \
    {         \
        pthread_cond_destroy(&(item->cond)); \
        free(item);     \
        pthread_rwlock_fcfs_queue_dequeue(rwlock->queue); \
    }

#define extract_first_non_disabled_item() \
    {     \
        /*       \
         * This loop extracts items from the queue until it encounters a  \
         * non-disabled item            \
         * */ \
        item = (pthread_rwlock_fcfs_item_t * )pthread_rwlock_fcfs_queue_peak(rwlock->queue);  \
        /*  \
         * As long as we did not reach the end of the queue and as long as the item   \
         * is disabled - destroy the item, dequeue it and peak the next item \
         * */   \
        while ((item != NULL) && (item->num_threads == 0))      \
        {       \
            dequeue_and_destroy_item(); \
            item = (pthread_rwlock_fcfs_item_t * )pthread_rwlock_fcfs_queue_peak(rwlock->queue);   \
        }      \
    }



/*
 * This function peaks and dequeues enough items from the queue in
 * order to accept the next round of pending items. By round, I mean
 * either an arbitrary number of readers or a single writer.
 * */
static void accept_pending_items(pthread_rwlock_fcfs_t * rwlock PTHREAD_RWLOCK_FCFS_DEBUG_ARGS)
{
    pthread_rwlock_fcfs_item_t * item;

    my_debug_print(rwlock, id, "accept_pending_items");

    extract_first_non_disabled_item();

    my_debug_print(rwlock, id, 
            (item
             ? "accept_pending - Valid item"
             : "accept_pending - Null Item"
             )
            );

    /* The queue is empty, so we don't need to release anything */
    if (item == NULL)
    {
        return;
    }

    my_debug_print(rwlock, id, "accept_pending_items - Item is not NULL");

    /* Check if it's a writer or a reader */
    if (item->is_writer)
    {
        my_debug_print(rwlock, id, "accept_pending_items - it's a writer");
        /* There is going to be one less writer waiting on the condition
         * variable */
        item->num_threads--;
        /* Prohibit other writers from joining this pack from now on */
        item->was_first_thr_accepted = 1;
        /* Release the writer. */
        rwlock->status = PTHREAD_RWLOCK_FCFS_USED_BY_A_WRITER;
        pthread_cond_signal( &(item->cond) );

        my_debug_print(rwlock, id, "accept_pending_items - signalled writer");

        /* Check if there are more threads pending. If not - we can extract
         * and destroy the item */
        if (item->num_threads == 0)
        {
            my_debug_print(rwlock, id, "accept_pending_items - wrt - dequeue");
            /* We don't need the item anymore so let's deallocate it */
            dequeue_and_destroy_item();
        }
    }
    else
    {
        my_debug_print(rwlock, id, "accept_pending_items - rd - broadcast");
        /* Release all the threads that are waiting on this item */

        rwlock->status = PTHREAD_RWLOCK_FCFS_USED_BY_READERS;
        rwlock->num_pending_readers = item->num_threads;
        pthread_cond_broadcast(& (item->cond) );

        my_debug_print(rwlock, id, "accept_pending_items - rd - after broadcast");

        /* Destroy and extract this pack */
        dequeue_and_destroy_item();

        my_debug_print(rwlock, id, "accept_pending_items - rd - dequeue");
    }
    /* I am doing it in order to remove junk from the queue. */
    extract_first_non_disabled_item();
}

/*
 * This functions eliminates disabled items from the head of the queue,
 * so pending readers will be able to be accepted immidiately.
 * */
static void remove_junk_from_head_of_queue(pthread_rwlock_fcfs_t * rwlock)
{
    pthread_rwlock_fcfs_item_t * item;

    extract_first_non_disabled_item();
}

#undef dequeue_and_destroy_item
#undef extract_first_non_disabled_item

/*
 * It is possible to determine whether the thread that releases the lock is a
 * reader or a writer according to the state of the lock.
 * */
extern void pthread_rwlock_fcfs_release(pthread_rwlock_fcfs_t * rwlock PTHREAD_RWLOCK_FCFS_DEBUG_ARGS)
{
    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Unlock!");

    /*
     * If there is a writer locking the lock, than obviously he is the one
     * that releases it.
     * */
    if (rwlock->status == PTHREAD_RWLOCK_FCFS_USED_BY_A_WRITER)
    {
        rwlock->status = PTHREAD_RWLOCK_FCFS_UNLOCKED;

        accept_pending_items(rwlock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
    }
    else
    {
        /*
         * Croak if there are also no readers - someone is trying to release
         * a lock which he does not have...
         * */
        assert(rwlock->num_readers > 0);

        rwlock->num_readers--;

        if (rwlock->num_readers == 0 && rwlock->num_pending_readers == 0)
        {
            rwlock->status = PTHREAD_RWLOCK_FCFS_UNLOCKED;
        }

        /*
         * Check if the lock is now free of readers. If so, we can accept
         * pending writers.
         * */

        if (rwlock->status == PTHREAD_RWLOCK_FCFS_UNLOCKED)
        {
            accept_pending_items(rwlock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);
        }
    }

    pthread_mutex_unlock(&(rwlock->mutex));
}

int
    pthread_rwlock_fcfs_timed_gain_write(
        pthread_rwlock_fcfs_t * rwlock,
        const struct timespec * abstime,
        int (*continue_callback)(void * context),
        void * context
        PTHREAD_RWLOCK_FCFS_DEBUG_ARGS
        )
{
    int ret;

    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Want Timed Write Lock!");

    ret = gain_write_generic(rwlock, 0, 1, 1, abstime, continue_callback, context);

    my_debug_print(rwlock, id, 
            ((ret == 0)
             ? "Gained Timed Write Lock!"
             : "Failed Timed Write Lock!"));

    pthread_mutex_unlock(&(rwlock->mutex));

    return ret;
}


static int gain_read_generic(
    pthread_rwlock_fcfs_t * rwlock,
    int wait_for_access,
    int is_timed,
    const struct timespec * abstime,
    int (*continue_callback)(void * context),
    void * context
    )
{
    /* If the rwlock is going to be destroyed - exit now. */
    if (rwlock->is_destroyed)
    {
        return -1;
    }
    /*
     * It is possible that there are some disabled items clogging the
     * queue, which will prevent this thread from being accepted immidiately.
     *
     * Therefore, try to remove them
     * */
    remove_junk_from_head_of_queue(rwlock);

    if ((rwlock->status != PTHREAD_RWLOCK_FCFS_USED_BY_A_WRITER) 
            && pthread_rwlock_fcfs_queue_is_empty(rwlock->queue))
    {
        /* Increment the number of readers. */
        rwlock->num_readers++;

        /* Now we have read access */

        return 0;
    }
    else
    {
        pthread_rwlock_fcfs_item_t * item;

        if (! wait_for_access)
        {
            return 1;
        }

        item = (pthread_rwlock_fcfs_item_t *)pthread_rwlock_fcfs_queue_peak_tail(rwlock->queue);
        if (
                /* The queue is not empty */
                (item != NULL) &&
                /* It is a readers' pack */
                (! item->is_writer)
           )
        {
            /* Use this item */
            item->num_threads++;
        }
        else
        {

            /* Let's enqueue myself */

            item = malloc(sizeof(pthread_rwlock_fcfs_item_t));
            /* We are a writer, so let's designate ourselves as such */
            item->is_writer = 0;

            /* We want to gain access to the lock */
            item->is_disabled = 0;

            /* This is a new pack with one thread waiting. */
            item->num_threads = 1;

            /* Initialize the condition variable */
            item->cond = initial_cond_constant;

            pthread_cond_init(&(item->cond), NULL);

            /* Let's put ourselves in the queue, so we will be released
             * when necessary. */
            pthread_rwlock_fcfs_queue_enqueue(rwlock->queue, item);
        }

        /* Wait upon the condition variable to gain access to the rwlock */
        if (! is_timed)
        {
            pthread_cond_wait(&(item->cond), &(rwlock->mutex));
        }
        else
        {
            int ret;

            /*
             * We try to get a timed wait on cond. If it times-out, we ask
             * the user if he wishes to continue.
             */
            do
            {
                ret = pthread_cond_timedwait(&(item->cond), &(rwlock->mutex), abstime);
            } while
                (
                    (ret == ETIMEDOUT) &&
                    (continue_callback != NULL) &&
                    continue_callback(context)
                );

            /* We failed for some reason (probably a time out - let's disable
             * this item, so it won't be processed.
             * */
            if (ret != 0)
            {
                /* Let the controlling thread know we are no longer waiting
                 * on the condition variable */

                item->num_threads--;
                return ret;
            }
        }

        /* Now we have a read access */
        rwlock->num_pending_readers--;
        rwlock->num_readers++;

        return 0;
    }
}

int pthread_rwlock_fcfs_timed_gain_read(
        pthread_rwlock_fcfs_t * rwlock,
        const struct timespec * abstime,
        int (*continue_callback)(void * context),
        void * context
        PTHREAD_RWLOCK_FCFS_DEBUG_ARGS
        )
{
    int ret;

    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Want Timed Read Lock!");

    ret = gain_read_generic(rwlock, 1, 1, abstime, continue_callback, context);

    my_debug_print(rwlock, id, 
            ((ret == 0) ? 
             "Failed Timed Read Lock!" : 
             "Failed Timed Read Lock!"
             )
            );

    pthread_mutex_unlock(&(rwlock->mutex));

    return ret;
}

int pthread_rwlock_fcfs_gain_read(pthread_rwlock_fcfs_t * rwlock PTHREAD_RWLOCK_FCFS_DEBUG_ARGS)
{
    int ret;

    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Want Read Lock!");

    ret = gain_read_generic(rwlock, 1, 0, NULL, NULL, NULL);

    my_debug_print(rwlock, id, "Gained Read Lock!");

    pthread_mutex_unlock(&(rwlock->mutex));

    return ret;
}

int pthread_rwlock_fcfs_try_gain_read(pthread_rwlock_fcfs_t * rwlock PTHREAD_RWLOCK_FCFS_DEBUG_ARGS)
{
    int ret;

    pthread_mutex_lock(&(rwlock->mutex));

    my_debug_print(rwlock, id, "Want Try Lock!");

    ret = gain_read_generic(rwlock, 0, 0, NULL, NULL, NULL);

    my_debug_print(rwlock, id, ((ret == 0) ? "Lock!" : "Failed Lock!"));

    pthread_mutex_unlock(&(rwlock->mutex));

    return ret;
}

extern void pthread_rwlock_fcfs_destroy(pthread_rwlock_fcfs_t * rwlock)
{
#ifdef PTHREAD_RWLOCK_FCFS_DEBUG
    char id[1] = "";
#endif
    /* Make sure no new threads are accepted */
    rwlock->is_destroyed = 1;

    /* Make sure all running threads are cleared up. */
    gain_write_for_destroy(rwlock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

    pthread_rwlock_fcfs_release(rwlock PTHREAD_RWLOCK_FCFS_DEBUG_CALL_ARGS);

    /* Now - destroy the lock. */
    pthread_rwlock_fcfs_queue_destroy(rwlock->queue);

    free(rwlock);

    return;
}

