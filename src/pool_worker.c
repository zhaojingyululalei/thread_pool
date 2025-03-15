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

    rb_node_t* node = rb_tree_find_by(&pool->worker_tree, tid, worker_compare);
    worker_t* worker = rb_node_parent(node,worker_t,rbnode);
    assert(worker != NULL); // 只要这个线程创建了，我肯定加入树中了
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
        if(!job->is_readed){
            //结果还没读的话，可以通知用户读取了
            semaphore_post(&job->ret_sem);
        }
        unlock(&job_lock);

        THREADPOOL_LOCK;
        //任务执行完毕
        log_message(LOGLEVEL_INFO, "worker:%u finish a job", tid);
        worker_set_idle(pool,worker);
        log_threadpool_status(pool);


    }
    log_message(LOGLEVEL_INFO, "worker:%u exist", tid);
    worker_set_cancle(pool,worker);
    log_threadpool_status(pool);
    THREADPOOL_UNLOCK;
    thread_exit();
    // if(!worker){
    //     //线程被创建后，可能还没加入idle list队列
    //     //可能还没运行就加入了cancle_list队列
    //    assert(worker!=NULL);

    // }
    // //只要没被取消，就一直执行
    // while (get_worker_state(pool,worker) != WORKER_STATE_CANCLE)
    // {
    //     //队列没有任务就等着
    //     semaphore_wait(&pool->job_sem);
    //     if(get_worker_state(pool,worker) == WORKER_STATE_CANCLE ){
    //         break;
    //     }
    //     lock(&pool->lock);
    //     //如果任务队列中没有任务
    //     if(pool->jobq.count<=0){
    //         unlock(&pool->lock);
    //         continue;
    //     }
    //     //有任务就取一个任务
    //     job_t *job;
    //     fixque_dequeue(&pool->jobq,&job);
    //     if(!job){
    //         dbg_error("job q no job\r\n");
    //     }
    //     //dbg_info("thread %u recv a task\r\n",tid);
    //     //取到了任务，worker改变状态,更换队列
    //     worker_set_busy(pool,worker);
    //     //dbg_info("idle list:\r\n");
    //     list_debug_print(&pool->idle_list);
    //     //dbg_info("busy list:\r\n");
    //     list_debug_print(&pool->busy_list);

    //     unlock(&pool->lock);

    //     job->ret = job->function(job->arg);
    //     semaphore_post(&job->ret_sem);

    //     lock(&pool->lock);
    //     //dbg_info("thread %u finish a task\r\n",tid);
    //     worker_set_idle(pool,worker);
    //     //dbg_info("idle list:\r\n");
    //     list_debug_print(&pool->idle_list);
    //     //dbg_info("busy list:\r\n");
    //     list_debug_print(&pool->busy_list);
    //     unlock(&pool->lock);
    // }
    // dbg_info("worker %u exit\r\n",tid);
    // thread_exit();
}