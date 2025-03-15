#ifndef __THREADPOOL_H
#define __THREADPOOL_H

#include "plat.h"
#include "fixque.h"
#include "list.h"
#include "rb_tree.h"
#include "threadpoolcfg.h"
#include "worker.h"
#include "job.h"

typedef struct
{
    enum
    {
        THREAD_POOL_STATE_CLOSE,
        THREAD_POOL_STATE_OPEN
    } state;
    lock_t lock;
    semaphore_t job_sem;   // 没有任务就等，有任务去获取
    fixque_t jobq;         // 任务队列
    list_t idle_list;      // 没取到任务的线程队列
    list_t busy_list;      // 取到任务的线程队列
    list_t cancle_list;    // 将要退出的线程
    rb_tree_t worker_tree; // 创建了的线程全部加入
    int thread_count;      // 当前线程数
    tid_t manager;         // 管理者线程tid
} threadpool_t;

int worker_set_idle(threadpool_t *pool, worker_t *worker);
int worker_set_busy(threadpool_t *pool, worker_t *worker);
int worker_set_cancle(threadpool_t *pool, worker_t *worker);

job_t *threadpool_submit_job(threadpool_t *pool, job_type_t type, void *(*function)(void *), void *arg, bool need_ret);
int threadpool_create_worker(threadpool_t *pool);
int threadpool_eliminate_workers(threadpool_t* pool,int n);
int threadpool_init(threadpool_t *pool);

void log_threadpool_status(threadpool_t *pool);
#endif