/*
 * queue.c
 * 
 * This module implements a queue
 *
 * */


#include <stdio.h>
#include <stdlib.h>

#include <pthread/rwlock_fcfs_queue.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

pthread_rwlock_fcfs_queue_t * pthread_rwlock_fcfs_queue_alloc(void)
{
    pthread_rwlock_fcfs_queue_t * ret;

    ret = malloc(sizeof(pthread_rwlock_fcfs_queue_t));

    ret->head = NULL;
    ret->tail = NULL;

    ret->num_msgs = 0;

    return ret;
}

void pthread_rwlock_fcfs_queue_destroy(pthread_rwlock_fcfs_queue_t * queue)
{
    free(queue);
}

void * pthread_rwlock_fcfs_queue_dequeue(pthread_rwlock_fcfs_queue_t * queue)
{
    pthread_rwlock_fcfs_queue_item_t * ret;
    void * void_ret;
    
    /* If there are no messages present in the queue */
    if (queue->head == NULL)
    {
        return NULL;
    }
    
    /* Retrieve the first element */
    ret = queue->head;
    /* Remove it from the list */
    queue->head = ret->next;
    /* Mark this list as empty if it is indeed so */
    if (queue->head == NULL)
    {
        queue->tail = NULL;
    }

    queue->num_msgs--;

    void_ret = ret->data;
    free(ret);
    
    return void_ret;
}

void * pthread_rwlock_fcfs_queue_peak(pthread_rwlock_fcfs_queue_t * queue)
{
    pthread_rwlock_fcfs_queue_item_t * ret;
    void * void_ret;
    
    /* If there are no messages present in the queue */
    if (queue->head == NULL)
    {
        return NULL;
    }
    
    /* Retrieve the first element */
    ret = queue->head;

    void_ret = ret->data;
    
    return void_ret;
}


void pthread_rwlock_fcfs_queue_enqueue(pthread_rwlock_fcfs_queue_t * queue, void * msg)
{
    pthread_rwlock_fcfs_queue_item_t * msg_with_next;

    msg_with_next = malloc(sizeof(pthread_rwlock_fcfs_queue_item_t));

    msg_with_next->data = msg;

    if (queue->tail == NULL)
    {                
        queue->head = queue->tail = msg_with_next;
        msg_with_next->next = NULL;
    }
    else
    {
        /* Append this message to the end of the linked list */
        queue->tail->next = msg_with_next;
        /* Mark it as the end */
        queue->tail = msg_with_next;
        /* Signify that it is not connected to anything */
        msg_with_next->next = NULL;
    }

    queue->num_msgs++;
}

int pthread_rwlock_fcfs_queue_is_empty(pthread_rwlock_fcfs_queue_t * queue)
{
    return (queue->head == NULL);
}

void * pthread_rwlock_fcfs_queue_peak_tail(pthread_rwlock_fcfs_queue_t * queue)
{
    pthread_rwlock_fcfs_queue_item_t * ret;
    void * void_ret;
    
    /* If there are no messages present in the queue */
    if (queue->tail == NULL)
    {
        return NULL;
    }
    
    /* Retrieve the first element */
    ret = queue->tail;

    void_ret = ret->data;
    
    return void_ret;
}




