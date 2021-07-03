//
// Created by 赵赵安安 on 2021/6/15.
//

#include "pthreadPool.h"

typedef struct Task{
    void (*function)(void *arg);
    void *arg;
}Task;

struct ThreadPool{
    //define task
    Task *TaskQ;
    int queueSize;
    int queueCapacity;
    int queueFront;
    int queueBack;

    pthread_t managerID;
    pthread_t *threadIDs;
    int minNum;
    int maxNum;
    int busyNum;
    int aliveNum;
    int exitNum;
    pthread_mutex_t mutexPool;
    pthread_mutex_t mutexBusy;
    pthread_cond_t notEmpty;
    pthread_cond_t notFull;

    int freeSign;
};

ThreadPool *creatThreadPool(int minNum, int maxNum, int queueSize){
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));

    do{
        if(pool == NULL){
            perror("pool malloc fail");
            break;
        }
        if((pool->threadIDs = (pthread_t *)malloc(sizeof(pthread_t) * maxNum)) == NULL){
            perror("pthread malloc fail");
            break;
        }

        memset(pool->threadIDs, 0, sizeof(pthread_t) * maxNum);
        pool->maxNum = maxNum;
        pool->minNum = minNum;
        pool->aliveNum = minNum;
        pool->busyNum = 0;
        pool->exitNum = 0;

        if(pthread_mutex_init(&pool->mutexBusy, NULL) || pthread_mutex_init(&pool->mutexPool, NULL) ||
           pthread_cond_init(&pool->notEmpty, NULL) || pthread_cond_init(&pool->notFull, NULL)){
            perror("mutex or cond init fail");
            break;
        }

        pool->TaskQ = (Task *)malloc(sizeof(Task) * queueSize);
        if(pool->TaskQ == NULL){
            perror("taskQ malloc fail");
            break;
        }
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueBack = 0;

        pool->freeSign = 0;

        if(pthread_create(&pool->managerID, NULL, manager, pool) == -1){
            perror("pthread_create manager fail");
            break;
        }
        for(int i = 0; i < pool->minNum; ++i){
            if(pthread_create(&pool->threadIDs[i], NULL, worker, pool) == -1){
                perror("pthread_create worker fail");
                break;
            }
        }
        return pool;
    }while(0);

    if(pool && pool->threadIDs) free(pool->threadIDs);
    if(pool && pool->TaskQ) free(pool->TaskQ);
    if(pool) free(pool);
}

void *worker(void *arg){
    ThreadPool *pool = (ThreadPool *)arg;

    while(1) {
        pthread_mutex_lock(&pool->mutexPool);
        while (!pool->queueSize && !pool->freeSign) {
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
            if(pool->exitNum){
                pool->exitNum--;
                if(pool->aliveNum > pool->minNum){
                    pool->aliveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    pthreadExit(pool);
                }
            }
        }

        if (pool->freeSign) {
            pthread_mutex_unlock(&pool->mutexPool);
            pthreadExit(pool);
            break;
        }

        Task task;
        task.function = pool->TaskQ[pool->queueFront].function;
        task.arg = pool->TaskQ[pool->queueFront].arg;

        pool->queueFront = ++(pool->queueFront) % pool->queueCapacity;
        pool->queueSize--;
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        printf("thread %ld start working .. \n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;
        printf("thread %ld end working .. \n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }

}

void *addTask(ThreadPool *pool, void (*func)(void *arg), void *arg){
    pthread_mutex_lock(&pool->mutexPool);
    while(pool->queueSize == pool->queueCapacity && !pool->freeSign){
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }

    if(pool->freeSign){
        pthread_mutex_unlock(&pool->mutexPool);
        return NULL;
    }

    pool->TaskQ[pool->queueBack].function = func;
    pool->TaskQ[pool->queueBack].arg = arg;
    pool->queueSize++;
    pool->queueBack = (pool->queueBack + 1) % pool->queueCapacity;

    pthread_cond_signal(&pool->notEmpty);
    pthread_mutex_unlock(&pool->mutexPool);
}

void *manager(void *arg){
    ThreadPool *pool = (ThreadPool *)arg;
    while(!pool->freeSign){
        sleep(3);

        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveSize = pool->aliveNum;
        int busySize = pool->busyNum;
        if(busySize > liveSize * 0.7 && liveSize < pool->maxNum){
            for(int i = 0, count = 0;count < 2 && pool->aliveNum < pool->maxNum; i = (i+1) % pool->maxNum){
                if(pool->threadIDs[i] == 0){
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    count++;
                    pool->aliveNum++;
                }
            }
        }
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexPool);
        queueSize = pool->queueSize;
        liveSize = pool->aliveNum;
        busySize = pool->busyNum;
        if(busySize < liveSize * 0.5 && liveSize < pool->maxNum){
            pool->exitNum = 2;
            for(int i = 0; i < 2; ++i){
                pthread_cond_signal(&pool->notEmpty);
            }
        }
        pthread_mutex_unlock(&pool->mutexPool);
    }
}

void pthreadExit(ThreadPool* pool){

    pthread_t pid = pthread_self();
    for(int i = 0; i < pool->maxNum; ++i){
        if(pool->threadIDs[i] == pid){
            pool->threadIDs[i] = 0;
            printf("destory thread\n");
            break;
        }
    }
    pthread_exit(NULL);
}

void poolDestory(ThreadPool* pool) {
    if(!pool){
        return;
    }
    pool->freeSign = 1;
    pthread_join(pool->managerID, NULL);
    for(int i = 0; i < pool->aliveNum; ++i){
        pthread_cond_signal(&pool->notEmpty);
    }
    if(pool->TaskQ){
        free(pool->TaskQ);
    }
    if(pool->threadIDs){
        free(pool->threadIDs);
    }
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
}

