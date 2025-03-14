#include "threadpool.h"
#include "debug.h"
static int get_job_cnt(threadpool_t* pool){
    int ret;
    lock(&pool->lock);
    ret = pool->jobq.count;
    unlock(&pool->lock);
    return ret;
}
static int get_busy_cnt(threadpool_t* pool){
    int ret;
    lock(&pool->lock);
    ret = pool->busy_list.count;
    unlock(&pool->lock);
    return ret;
}
static int get_idle_cnt(threadpool_t* pool){
    int ret;
    lock(&pool->lock);
    ret = pool->idle_list.count;
    unlock(&pool->lock);
    return ret;
}
/**
 * @brief 如果任务数量是busy线程的2倍多,就增加线程数量
 */
static void manager_add_workers(threadpool_t* pool){
    
    int job_cnt = get_job_cnt(pool);
    int busy_cnt = get_busy_cnt(pool);
    int add_cnt = 0;

    if(job_cnt >= 2 * busy_cnt){
        
        add_cnt = (job_cnt - busy_cnt)/2;
        for (int i = 0; i < add_cnt; i++)
        {
            threadpool_create_worker(pool);
        }
        
    }
    lock(&pool->lock);
    dbg_info("manager add %d worker\r\n",add_cnt);
    pool_debug_all_list(pool);
    unlock(&pool->lock);
    //否则不用添加
}
/**
 * @brief 如果空闲线程是busy线程的2倍多，就减少一半的空闲线程
 */
static void manager_eliminate_workers(threadpool_t* pool){
    int busy_cnt = get_busy_cnt(pool);
    int idle_cnt = get_idle_cnt(pool);
    int elim_cnt = idle_cnt / 2;
    if(idle_cnt >= 2* busy_cnt && idle_cnt > THREAD_POOL_MIN_SIZE){
        
        
        threadpool_eliminate_idle(pool,elim_cnt);
    }
    lock(&pool->lock);
    dbg_info("manager eliminate %d worker\r\n",elim_cnt);
    pool_debug_all_list(pool);
    unlock(&pool->lock);
}
/**
 * @brief 回收一些cancle_list中的线程
 */

static void manager_collect_workers(threadpool_t* pool){
    lock(&pool->lock);
    
    if(pool->cancle_list.count == 0){
        unlock(&pool->lock);
        return;
    }
    for (int i = 0; i < pool->cancle_list.count; i++)
    {
        list_node_t* first = list_remove_first(&pool->cancle_list);
        worker_t* w = list_node_parent(first,worker_t,node);
        thread_join(w->tid);
        dbg_info("manager collec %u worker\r\n",w->tid);
        worker_free(w);
    }
    pool_debug_all_list(pool);
    unlock(&pool->lock);
    
}
/**
 * @brief 回收任务块
 */
static void manager_collect_jobs(threadpool_t* pool){
    lock(&pool->lock);
    int count = 0;
    for (int i = 0; i < MAX_JOB_CNT; i++)
    {
        job_t* job = get_job_by_idx(i);
        if(job->type!= JOB_TYPE_NONE && job->is_readed){
            count++;
            job_free(job);
        }
    }
    dbg_info("manager collect %d job_T",count);
    unlock(&pool->lock);
}

void* pool_manager(void* arg){
    threadpool_t* pool = (threadpool_t*)arg;

    //线程池只要没销毁，就一直执行
    while (get_threadpool_state(pool) == THREAD_POOL_STATE_OPEN)
    {
        //判断是否需要增加线程
        manager_add_workers(pool);
        //判断是否需要消灭线程
        manager_eliminate_workers(pool);

        //manager线程也不用一直扫描整个线程池做调整，隔一段时间扫就可以
        sleep(1); 

        //回收一些cancle_list里面的线程
        manager_collect_workers(pool);
        //回收一些job_t
        manager_collect_jobs(pool);
    }
    thread_exit();
}