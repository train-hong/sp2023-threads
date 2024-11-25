#ifndef __MY_THREAD_POOL_H
#define __MY_THREAD_POOL_H

#include <pthread.h>

typedef struct tpool_task {
    void *(*func)(void *);
    void *arg;
    struct tpool_task *next;
} tpool_task;

typedef struct tpool {
    tpool_task *queue_head;
    tpool_task *queue_tail;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_empty;
    pthread_t *threads;
    int num_threads;
    int stop;
} tpool;

tpool *tpool_init(int num_threads);
void tpool_add(tpool *pool, void *(*func)(void *), void *arg);
void tpool_wait(tpool *pool);
void tpool_destroy(tpool *pool);

#endif
