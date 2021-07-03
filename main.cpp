#include "pthreadPool.h"

void taskFunc(void* arg)
{
    int num = *(int*)arg;
    printf("thread %ld is working, number = %d\n",
           pthread_self(), num);
    sleep(1);
}

int main()
{
    // 创建线程池
    ThreadPool* pool = creatThreadPool(3, 20, 100);
    for (int i = 0; i < 5; ++i)
    {
        int *num = (int*)malloc(sizeof(int));
        *num = i;
        addTask(pool, taskFunc, num);
    }

    sleep(30);

    poolDestory(pool);
    return 0;
}


