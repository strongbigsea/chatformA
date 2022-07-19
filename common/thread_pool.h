/*************************************************************************
	> File Name: thread_pool.h
	> Author: 
	> Mail: 
	> Created Time: Wed 13 Jul 2022 11:17:33 AM CST
 ************************************************************************/

#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
struct task_queue{
    int head, tail, total, size;
    void **data;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};
void task_queue_init(struct task_queue* taskQueue, int size);
void task_queue_push(struct task_queue* taskQueue, void* data);
void* task_queue_pop(struct task_queue* taskQueue);

void* pthread_run(void *arg);
void* worker(void *arg);
#endif
