#include <stdio.h>
#include <stdlib.h>

#include <iostream.h>

#include <pthread/rwlock_fcfs_queue.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int main()
{
    pthread_rwlock_fcfs_queue_t * queue;

    queue = pthread_rwlock_fcfs_queue_alloc();

    int element;
    int * ptr;

    char op;

    while(!cin.eof())
    {
        cout << "Op:\n";
        cin >> op;
        if (op == 'e')
        {
            cin >> element;
            ptr = (int*)malloc(sizeof(int));
            *ptr = element;
            pthread_rwlock_fcfs_queue_enqueue(queue, ptr);
        }
        else if (op == 'd')
        {
            ptr = (int *)pthread_rwlock_fcfs_queue_dequeue(queue);
            if (ptr == NULL)
            {
                cout << "Queue is empty!\n";
            }
            else
            {
                cout << "DeQed " << *ptr << "\n";
                free(ptr);
            }
        }
        else if (op == 'p')
        {
            ptr = (int *)pthread_rwlock_fcfs_queue_poll(queue);
            if (ptr == NULL)
            {
                cout << "Queue is empty!\n";
            }
            else
            {
                cout << "Polled " << *ptr << "\n";
            }
        }
    }

    return 0;
}
