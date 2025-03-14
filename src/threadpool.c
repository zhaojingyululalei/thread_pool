

#include "threadpool.h"
#include <string.h>
#include "debug.h"

extern void *pool_worker(void *arg);
extern void *pool_manager(void *arg);

static worker_t worker_buf[THREAD_POOL_MAX_SIZE];
 worker_t *worker_alloc(void)
{
    for (int i = 0; i < THREAD_POOL_MAX_SIZE; i++)
    {
        if (worker_buf[i].state == WORKER_STATE_NONE)
        {
            worker_buf[i].state = WORKER_STATE_IDLE;
            return &worker_buf[i];
        }
    }
    dbg_error("no more worker\r\n");
}
 void worker_free(worker_t *worker)
{
    worker->state = WORKER_STATE_NONE;
}

static job_t job_buf[MAX_JOB_CNT];
job_t* get_job_by_idx(int idx){
    if(idx >= MAX_JOB_CNT){
        return NULL;
    }
    job_t* job =  &job_buf[idx];
    return job;
}
static job_t *job_alloc(job_type_t type)
{
    
    for (int i = 0; i < MAX_JOB_CNT; i++)
    {
        if (job_buf[i].type == JOB_TYPE_NONE)
        {
            job_buf[i].type = type;
            return &job_buf[i];
        }
    }
    dbg_error("no more job\r\n");
}
/**
 * @brief 由用户释放
 */
void job_free(job_t *job)
{
    job->type = JOB_TYPE_NONE;
    semaphore_destroy(&job->ret_sem);
}
/**
 * @brief 创建管理线程
 */
static int threadpool_create_manager(threadpool_t *pool)
{
    pool->manager = thread_create(pool_manager, pool);
    return 0;
}

/**
 * @brief 线程池创建一个工作线程,并加入空闲队列
 */
int threadpool_create_worker(threadpool_t *pool)
{
    lock(&pool->lock);
    tid_t tid = thread_create(pool_worker, pool);

    worker_t *worker = worker_alloc();
    if (!worker)
    {
        unlock(&pool->lock);
        return -1;
    }
    worker->tid = tid;
    worker->state = WORKER_STATE_IDLE;
    list_insert_last(&pool->idle_list, &worker->node);
    dbg_info("create worker %u\r\n", tid);
    unlock(&pool->lock);
    return 0;
}
/**
 * @brief 根据tid找到对应的worker结构
 */
worker_t *find_worker_by_tid(threadpool_t *pool, tid_t tid)
{
    lock(&pool->lock);

    list_node_t *cur = pool->idle_list.first;
    while (cur)
    {
        worker_t *worker = list_node_parent(cur, worker_t, node);
        if (worker->tid == tid)
        {
            unlock(&pool->lock);
            return worker;
        }
        cur = cur->next;
    }
    cur = pool->busy_list.first;
    while (cur)
    {
        worker_t *worker = list_node_parent(cur, worker_t, node);
        if (worker->tid == tid)
        {
            unlock(&pool->lock);
            return worker;
        }
        cur = cur->next;
    }
    cur = pool->cancle_list.first;
    while (cur)
    {
        worker_t *worker = list_node_parent(cur, worker_t, node);
        if (worker->tid == tid)
        {
            unlock(&pool->lock);
            return worker;
        }
        cur = cur->next;
    }
    unlock(&pool->lock);
    return NULL;
}

worker_state_t get_worker_state(threadpool_t *pool, worker_t *worker)
{
    worker_state_t state;
    lock(&pool->lock);
    state = worker->state;
    unlock(&pool->lock);
    return state;
}
int get_threadpool_state(threadpool_t *pool)
{
    int state;
    lock(&pool->lock);
    state = pool->state;
    unlock(&pool->lock);
    return state;
}

/**
 * @brief 工作线程执行完任务后，状态变为idle
 */
int worker_set_idle(threadpool_t *pool, worker_t *worker)
{
    assert(worker->state == WORKER_STATE_BUSY);
    worker->state = WORKER_STATE_IDLE;
    list_remove(&pool->busy_list, &worker->node);
    list_insert_last(&pool->idle_list, &worker->node);
    return 0;
}
/**
 * @brief 工作线程获取任务后，状态变为busy
 */
int worker_set_busy(threadpool_t *pool, worker_t *worker)
{
    assert(worker->state == WORKER_STATE_IDLE);
    worker->state = WORKER_STATE_BUSY;
    list_remove(&pool->idle_list, &worker->node);
    list_insert_last(&pool->busy_list, &worker->node);
    return 0;
}
/**
 * @brief 工作线程取消，状态变为cancle
 */
int worker_set_cancle(threadpool_t *pool, worker_t *worker)
{
    assert(worker->state == WORKER_STATE_IDLE);
    worker->state = WORKER_STATE_CANCLE;
    list_remove(&pool->idle_list, &worker->node);
    list_insert_last(&pool->cancle_list, &worker->node);
    return 0;
}

/**
 * @brief 消灭n个空闲线程
 */
int threadpool_eliminate_idle(threadpool_t *pool, int n)
{
    int ret;
    lock(&pool->lock);
    int idle_cnt = pool->idle_list.count;
    if(n >= idle_cnt){
        //如果消灭的太多了，那就不消灭了
        unlock(&pool->lock);
        return -1;
    }
    //搬运要取消的一些线程到cancle_list
    for (int i = 0; i < n; i++)
    {
        list_node_t *first = pool->idle_list.first;
        worker_t *w = list_node_parent(first, worker_t, node);
        worker_set_cancle(pool,w);
    }

    //把原先idle_list中的线程全部唤醒
    for (int i = 0; i < idle_cnt; i++)
    {
        semaphore_post(&pool->job_sem);
    }
    
    unlock(&pool->lock);
    return ret;
}
/**
 * @brief 向线程池提交一个任务
 * @param need_ret:是否需要返回值，不需要返回值的话运行完job直接删除job_t；
 *                  需要返回值的话，用户调用get_job_ret后，由manager线程删除job_t
 */
job_t *threadpool_submit_job(threadpool_t *pool, job_type_t type, void *(*function)(void *), void *arg,bool need_ret)
{
    int ret;
    lock(&pool->lock);
    job_t *job = job_alloc(type);
    job->function = function;
    job->arg = arg;
    job->ret = NULL;
    semaphore_init(&job->ret_sem, 0);
    if(need_ret){
        job->is_readed = false; //用户还没有读返回值
    }else{
        job->is_readed = true; //运行完job直接删
    }
    unlock(&pool->lock);
    switch (type)
    {
    case JOB_TYPE_ONCE:
        // 加入jobq
        lock(&pool->lock);
        ret = fixque_enqueue(&pool->jobq, job);

        unlock(&pool->lock);
        break;

    default:
        dbg_error("unkown job type\r\n");
        break;
    }
    assert(ret >= 0);
    semaphore_post(&pool->job_sem);
    return job;
}
/**
 * @brief 获取job的返回值(如果有的话)
 */
void *get_job_ret(threadpool_t* pool,job_t *job)
{
    semaphore_wait(&job->ret_sem);
    lock(&pool->lock);
    job->is_readed = true;
    unlock(&pool->lock);
    return job->ret;
}

// 线程池初始化
int threadpool_init(threadpool_t *pool, void **task_buf, int job_max_cnt)
{
    memset(pool, 0, sizeof(threadpool_t));

    lock_init(&pool->lock);

    semaphore_init(&pool->job_sem, 0); // 一开始任务队列没有任务

    fixque_init(&pool->jobq, task_buf, job_max_cnt);

    list_init(&pool->idle_list);
    list_init(&pool->busy_list);
    list_init(&pool->cancle_list);

    

    // 创建工作线程
    for (int i = 0; i < THREAD_POOL_MIN_SIZE; i++)
    {
        threadpool_create_worker(pool);
    }

    pool->thread_count = THREAD_POOL_MIN_SIZE;
    pool->idle_count = THREAD_POOL_MIN_SIZE;
    pool->busy_count = 0;

    pool->state = THREAD_POOL_STATE_OPEN;

    // 创建管理线程
    threadpool_create_manager(pool);
}

void worker_debug_print(worker_t *worker)
{
    if (worker->state == WORKER_STATE_IDLE)
    {
        dbg_info("worker tid:%u,state is idle\r\n", worker->tid);
    }
    else if (worker->state == WORKER_STATE_BUSY)
    {
        dbg_info("worker tid:%u,state is busy\r\n", worker->tid);
    }
    else
    {
        dbg_error("worker tid:%u,state is none\r\n", worker->tid);
    }
}

void list_debug_print(list_t *list)
{
    list_node_t *cur = list->first;
    while (cur)
    {
        worker_t *worker = list_node_parent(cur, worker_t, node);
        worker_debug_print(worker);
        cur = cur->next;
    }
}
void pool_debug_all_list(threadpool_t* pool){
    dbg_info("******idle list******\r\n");
    list_debug_print(&pool->idle_list);
    dbg_info("*********************\r\n");

    dbg_info("******busy list******\r\n");
    list_debug_print(&pool->busy_list);
    dbg_info("*********************\r\n");

    dbg_info("******cancle list******\r\n");
    list_debug_print(&pool->cancle_list);
    dbg_info("*********************\r\n");
}