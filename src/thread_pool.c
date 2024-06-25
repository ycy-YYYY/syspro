/** thread pool */
#include "thread_pool.h"
#include "log.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#define MAX_THREAD_NUM 100
typedef struct Task
{
    void *(*task)(void *arg);
    void *arg;
    ListNode list;
    bool is_exit;
} Task;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 互斥锁保护任务队列
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;    // 条件变量唤醒线程

int thread_num = 1;
ListNode task_list; // 任务队列
pthread_t tid[MAX_THREAD_NUM];

static void *thread_func(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        while (list_empty(&task_list))
        {
            pthread_cond_wait(&cond, &mutex);
        }
        Task *t = list_entry(task_list.next, Task, list);
        if (t->is_exit)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }
        list_del(&t->list);
        pthread_mutex_unlock(&mutex);
        t->task(t->arg);
        free(t);
    }
    return NULL;
}

void thread_pool_init(int num)
{
    thread_num = num;
    INIT_LIST_HEAD(&task_list);
    for (int i = 0; i < thread_num; i++)
    {
        pthread_t pid;
        LOG_TP("Create thread %d\n", i);
        pthread_create(&pid, NULL, thread_func, NULL);
        tid[i] = pid;
    }
}

void thread_pool_add_task(void *(*task)(void *arg), void *arg)
{
    Task *t = (Task *)malloc(sizeof(Task));
    t->task = task;
    t->arg = arg;
    t->is_exit = false;
    pthread_mutex_lock(&mutex);
    list_add(&t->list, &task_list);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void thread_pool_destroy()
{
    pthread_mutex_lock(&mutex);
    Task *t = (Task *)malloc(sizeof(Task));
    t->is_exit = true;
    list_add(&t->list, &task_list);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < thread_num; i++)
    {
        LOG_TP("Join thread %d\n", i);
        pthread_join(tid[i], NULL);
    }
    ListNode *pos;
    list_for_each(pos, &task_list)
    {
        Task *t = list_entry(pos, Task, list);
        free(t);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}