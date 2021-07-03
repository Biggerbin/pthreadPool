//
// Created by 赵赵安安 on 2021/6/15.
//

#ifndef PTHREADPOOL_PTHREADPOOL_H
#define PTHREADPOOL_PTHREADPOOL_H
#include <unistd.h>
#include <pthread.h>
#include <mutex>
#include <stdlib.h>

typedef struct ThreadPool ThreadPool;

ThreadPool *creatThreadPool(int minNum, int maxNum, int queueSize);

void *worker(void *arg);

void *addTask(ThreadPool *pool, void (*func)(void *arg), void *arg);

void *manager(void *arg);

void pthreadExit(ThreadPool* pool);

void poolDestory(ThreadPool* pool);
#endif //PTHREADPOOL_PTHREADPOOL_H
