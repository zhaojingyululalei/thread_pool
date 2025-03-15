#include "threadpool.h"
#include "job.h"
#include "worker.h"
#include "log.h"
#include <assert.h>
#include "debug.h"
extern void* pool_worker(void*);
extern void* pool_manager(void*);



/**
 * @brief 工作线程执行完任务后，状态变为idle
 */
int worker_set_idle(threadpool_t *pool, worker_t *worker)
{
    assert(worker->state == WORKER_STATE_BUSY);
    worker->state = WORKER_STATE_IDLE;
    list_remove(&pool->busy_list, &worker->lnode);
    list_insert_last(&pool->idle_list, &worker->lnode);
    return 0;
}
/**
 * @brief 工作线程获取任务后，状态变为busy
 */
int worker_set_busy(threadpool_t *pool, worker_t *worker)
{
    assert(worker->state == WORKER_STATE_IDLE);
    worker->state = WORKER_STATE_BUSY;
    list_remove(&pool->idle_list, &worker->lnode);
    list_insert_last(&pool->busy_list, &worker->lnode);
    return 0;
}
/**
 * @brief 工作线程取消，状态变为cancle
 */
int worker_set_cancle(threadpool_t *pool, worker_t *worker)
{
    //被唤醒后，先检查cancle标志，还没取任务，因此此时状态位idle才对
    assert(worker->state == WORKER_STATE_IDLE);
    worker->state = WORKER_STATE_CANCLE;
    list_remove(&pool->idle_list, &worker->lnode);
    list_insert_last(&pool->cancle_list, &worker->lnode);
    return 0;
}

/**
 * @brief 向线程池提交一个任务
 * @param need_ret:是否需要返回值，不需要返回值的话运行完job直接删除job_t；
 *                  需要返回值的话，用户调用get_job_ret后，由manager线程删除job_t
 */
job_t *threadpool_submit_job(threadpool_t *pool, job_type_t type, void *(*function)(void *), void *arg,bool need_ret)
{
    int ret;
    lock(&job_lock);
    
    job_t *job = job_alloc();
    job->type = type;
    job->function = function;
    job->arg = arg;
    job->ret = NULL;
    semaphore_init(&job->ret_sem, 0);
    if(need_ret){
        job->is_readed = false; //用户还没有读返回值
    }else{
        job->is_readed = true; //运行完job直接删
    }
    unlock(&job_lock);
    switch (type)
    {
    case JOB_TYPE_ONCE:
        // 加入jobq
        
        ret = fixque_enqueue(&pool->jobq, job);
        assert(ret >= 0);
        break;

    default:
        dbg_error("unkown job type\r\n");
        break;
    }
  
    semaphore_post(&pool->job_sem);
    return job;
}
/**
 * @brief 创建工作线程
 */
int threadpool_create_worker(threadpool_t* pool){
    tid_t tid = thread_create(pool_worker, pool);

    worker_t *worker = worker_alloc();
    assert(worker!=NULL);
    worker->tid = tid;
    worker->state = WORKER_STATE_IDLE;
    //将线程加入idle队列
    list_insert_last(&pool->idle_list, &worker->lnode);
    //将线程加入rbtree
    rb_tree_insert(&pool->worker_tree,worker);
    pool->thread_count++;
    log_message(LOGLEVEL_INFO,"create one worker thread:%u",tid);
    return 0;
}


/**
 * @brief 线程池初始化
 */
int threadpool_init(threadpool_t *pool)
{
    memset(pool, 0, sizeof(threadpool_t));

    log_init("./log.txt",1024*1024);

    
    job_init();

    worker_init();

    lock_init(&pool->lock);

    semaphore_init(&pool->job_sem, 0); // 一开始任务队列没有任务

    fixque_init(&pool->jobq, job_buffer, MAX_JOB_CNT);

    list_init(&pool->idle_list);
    list_init(&pool->busy_list);
    list_init(&pool->cancle_list);

    rb_tree_init(&pool->worker_tree,THREAD_POOL_MAX_SIZE,worker_compare,worker_get_node,node_get_worker);

    lock(&pool->lock);
    
    // 创建工作线程
    for (int i = 0; i < THREAD_POOL_MIN_SIZE; i++)
    {
        threadpool_create_worker(pool);
    }
    log_message(LOGLEVEL_INFO,"threadpool create %d init worker thread",THREAD_POOL_MIN_SIZE);
    log_threadpool_status(pool);
    pool->thread_count = THREAD_POOL_MIN_SIZE;
    pool->idle_count = THREAD_POOL_MIN_SIZE;
    pool->busy_count = 0;

    pool->state = THREAD_POOL_STATE_OPEN;

    // 创建管理线程
    //threadpool_create_manager(pool);
    unlock(&pool->lock);
    return 0;
}

void log_threadpool_status(threadpool_t* pool) {
#ifdef THREADPOOL_LOG
    

    log_message(LOGLEVEL_DEBUG, "Thread Pool State: %s, Threads: %d, Idle: %d, Busy: %d, Cancelling: %d,Tree:%d",
                (pool->state == THREAD_POOL_STATE_OPEN) ? "OPEN" : "CLOSED",
                pool->thread_count, list_count(&pool->idle_list), list_count(&pool->busy_list), list_count(&pool->cancle_list),pool->worker_tree.count);

    log_worker_list("Idle List", &pool->idle_list);
    log_worker_list("Busy List", &pool->busy_list);
    log_worker_list("Cancle List", &pool->cancle_list);

    
#endif
}