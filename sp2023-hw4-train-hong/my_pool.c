#include "my_pool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *tpool_worker(void *arg);

tpool *tpool_init(int num_threads) {
    tpool *pool = (tpool *)malloc(sizeof(tpool));
    pool->num_threads = num_threads;
    pool->threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->stop = 0;

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_not_empty, NULL);
    pthread_cond_init(&pool->queue_empty, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&(pool->threads[i]), NULL, tpool_worker, pool);
    }

    return pool;
}

void *tpool_worker(void *arg) {
    tpool *pool = (tpool *)arg;
    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);
        // printf("worker lock\n");
        while (pool->queue_head == NULL && !pool->stop) {
            pthread_cond_wait(&pool->queue_not_empty, &pool->queue_mutex);
        }
        // printf("thread id = %lu\n", pthread_self());
        // if (pool->stop) printf("pool->stop = %d\n", pool->stop);
        // if (pool->queue_head == NULL) printf("head is NULL\n");
        if (pool->stop && pool->queue_head == NULL) {
            pthread_mutex_unlock(&pool->queue_mutex);
            // printf("worker unlock\n");
            pthread_exit(NULL);
        }

        tpool_task *task = pool->queue_head;
        if (task->next == NULL) {
            // printf("a\n");
            pool->queue_head = NULL;
            pool->queue_tail = NULL;
        }
        else {
            // printf("b\n");
            pool->queue_head = task->next;
        }
        // if (pool->queue_head == NULL) printf("head is NULL 2\n");
        
        
        pthread_cond_broadcast(&pool->queue_not_empty);
        pthread_mutex_unlock(&pool->queue_mutex);
        // printf("worker unlock\n");

        if (task != NULL) {
            task->func(task->arg);
            free(task);
        }        
    }
}

void tpool_add(tpool *pool, void *(*func)(void *), void *arg) {
    // create task
    tpool_task *task = (tpool_task *)malloc(sizeof(tpool_task));
    task->func = func;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->queue_mutex);
    // printf("add lock\n");

    if (pool->queue_head == NULL) {
        pool->queue_head = task;
        pool->queue_tail = task;
        pthread_cond_signal(&pool->queue_not_empty);
    } else {
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }

    pthread_mutex_unlock(&pool->queue_mutex);
    // printf("add unlock\n");

}

void tpool_wait(tpool *pool) {
    pthread_mutex_lock(&pool->queue_mutex);
    // printf("wait lock\n");
    pool->stop = 1;
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);
    // printf("wait unlock\n");


    for (int i = 0; i < pool->num_threads; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            perror("Error joining thread");
            exit(EXIT_FAILURE);
        }
        // printf("Thread %d joined\n", i);
    }
    // printf("t");
}

void tpool_destroy(tpool *pool) {
    free(pool->threads);
    free(pool);
}
