

#ifndef __PTHREAD_RWLOCK_FCFS_QUEUE_H
#define __PTHREAD_RWLOCK_FCFS_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#define IP_NOISE_MESSAGE_BUFSIZE 0x20000

#ifndef __KERNEL__

struct pthread_rwlock_fcfs_queue_item_struct
{
    void * data;
    struct pthread_rwlock_fcfs_queue_item_struct * next;
};

typedef struct pthread_rwlock_fcfs_queue_item_struct pthread_rwlock_fcfs_queue_item_t;

struct pthread_rwlock_fcfs_queue_struct
{
    pthread_rwlock_fcfs_queue_item_t * head;
    pthread_rwlock_fcfs_queue_item_t * tail;

    int num_msgs;
};

typedef struct pthread_rwlock_fcfs_queue_struct pthread_rwlock_fcfs_queue_t;

extern pthread_rwlock_fcfs_queue_t * pthread_rwlock_fcfs_queue_alloc(void);

extern void pthread_rwlock_fcfs_queue_destroy(pthread_rwlock_fcfs_queue_t * queue);

extern void * pthread_rwlock_fcfs_queue_dequeue(pthread_rwlock_fcfs_queue_t * queue);

extern void * pthread_rwlock_fcfs_queue_peak(pthread_rwlock_fcfs_queue_t * queue);

extern void pthread_rwlock_fcfs_queue_enqueue(pthread_rwlock_fcfs_queue_t * queue, void * msg);

extern int pthread_rwlock_fcfs_queue_is_empty(pthread_rwlock_fcfs_queue_t * queue);

extern void * pthread_rwlock_fcfs_queue_peak_tail(pthread_rwlock_fcfs_queue_t * queue);


#endif


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __PTHREAD_RWLOCK_FCFS_QUEUE_H */

