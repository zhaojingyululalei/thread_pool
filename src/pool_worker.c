#include "threadpool.h"
#include "debug.h"
void* pool_worker(void* arg){
    threadpool_t* pool = (threadpool_t*)arg;
    tid_t tid = gettid(); //获取当前线程的线程id
    worker_t* worker = find_worker_by_tid(pool,tid);
    if(!worker){
        //线程被创建后，可能还没加入idle list队列
        //可能还没运行就加入了cancle_list队列
       assert(worker!=NULL);
        
    }
    //只要没被取消，就一直执行
    while (get_worker_state(pool,worker) != WORKER_STATE_CANCLE)
    {
        //队列没有任务就等着
        semaphore_wait(&pool->job_sem);
        if(get_worker_state(pool,worker) == WORKER_STATE_CANCLE ){
            break;
        }
        lock(&pool->lock);
        //如果任务队列中没有任务
        if(pool->jobq.count<=0){
            unlock(&pool->lock);
            continue;
        }
        //有任务就取一个任务
        job_t *job;
        fixque_dequeue(&pool->jobq,&job);
        if(!job){
            dbg_error("job q no job\r\n");
        }
        //dbg_info("thread %u recv a task\r\n",tid);
        //取到了任务，worker改变状态,更换队列
        worker_set_busy(pool,worker);
        //dbg_info("idle list:\r\n");
        list_debug_print(&pool->idle_list);
        //dbg_info("busy list:\r\n");
        list_debug_print(&pool->busy_list);

        unlock(&pool->lock);
        
        job->ret = job->function(job->arg);
        semaphore_post(&job->ret_sem);


        lock(&pool->lock);
        //dbg_info("thread %u finish a task\r\n",tid);
        worker_set_idle(pool,worker);
        //dbg_info("idle list:\r\n");
        list_debug_print(&pool->idle_list);
        //dbg_info("busy list:\r\n");
        list_debug_print(&pool->busy_list);
        unlock(&pool->lock);
    }
    dbg_info("worker %u exit\r\n",tid);
    thread_exit();
}