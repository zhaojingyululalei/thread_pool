#include "threadpool.h"
#include "job.h"
#include "worker.h"
#include "debug.h"
#include "log.h"
#define THREADPOOL_LOCK lock(&pool->lock)
#define THREADPOOL_UNLOCK unlock(&pool->lock)
void *pool_worker(void *arg)
{
    int ret;
    threadpool_t *pool = (threadpool_t *)arg;

    tid_t tid = gettid(); // 获取当前线程的线程id
    THREADPOOL_LOCK;

    rb_node_t* node = rb_tree_find_by(&pool->worker_tree, &tid, worker_compare_by_key);
    worker_t* worker = rb_node_parent(node,worker_t,rbnode);
    if(!worker){

        assert(worker!=NULL);
    }
    while (worker->state != WORKER_STATE_CANCLE)
    {
        
        if (!pool->jobq.count)
        {
            THREADPOOL_UNLOCK;
            semaphore_wait(&pool->job_sem); // 主要目的是让出cpu，并不是判断就绪队列是否有任务
            THREADPOOL_LOCK;
        }

        if (worker->state == WORKER_STATE_CANCLE)
        {

            break;
        }

        // 还是没有任务
        if (!pool->jobq.count)
        {
            continue;
        }

        // 有任务就取一个任务
        job_t *job;
        ret = fixque_dequeue(&pool->jobq, &job);
        if (ret < 0)
        {
            dbg_warning("jobq no job\r\n");
        }

        log_message(LOGLEVEL_INFO, "worker:%u get a job", tid);
        assert(tid == worker->tid);
        worker_set_busy(pool, worker);
        log_threadpool_status(pool);

        THREADPOOL_UNLOCK;

        //取出任务执行，并通知用户可以读结果了
        void* ret = job->function(job->arg);

        lock(&job_lock);
        job->ret = ret;
        if(job->is_needret){
            //如果用户需要读取返回值，那么就通知用户去读。用户读完后，设置job.isdone
            semaphore_post(&job->ret_sem);
        }else{
            //如果用户不需要返回值，直接设置is_done，manger回收Job_t
            job->is_done = true;
        }
        
        unlock(&job_lock);

        THREADPOOL_LOCK;
        //任务执行完毕
        log_message(LOGLEVEL_INFO, "worker:%u finish a job", tid);
        worker_set_idle(pool,worker);
        log_threadpool_status(pool);


    }
    log_message(LOGLEVEL_INFO, "worker:%u exist", tid);
    worker->exsit_done = true;
    log_threadpool_status(pool);
    THREADPOOL_UNLOCK;
    thread_exit();
    
}