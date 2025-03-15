
#include "threadpool.h"
#include "job.h"
#include "worker.h"
#include "debug.h"
#include "log.h"
#define THREADPOOL_LOCK lock(&pool->lock)
#define THREADPOOL_UNLOCK unlock(&pool->lock)

/**
 * @brief 返回要增加的线程数量
 */
int is_add_worker(threadpool_t* pool){
    int job_cnt = pool->jobq.count;
    int busy_cnt = list_count(&pool->busy_list);
    int idle_cnt = list_count(&pool->idle_list);
    int add_cnt = 0;

    if(job_cnt >= 2 * busy_cnt && idle_cnt==0){
        add_cnt = busy_cnt/2; 
    }
    return add_cnt;
}
/**
 * @brief 返回要减少的线程数
 */
int is_desc_worker(threadpool_t* pool){
    int job_cnt = pool->jobq.count;
    int busy_cnt = list_count(&pool->busy_list);
    int idle_cnt = list_count(&pool->idle_list);
    int desc_cnt = 0;

    if(job_cnt ==0 && idle_cnt * 2 >= pool->thread_count){
        desc_cnt = idle_cnt / 2;
    }
    return desc_cnt;

}
void* pool_manager(void* arg){
    threadpool_t* pool = (threadpool_t*)arg;
    THREADPOOL_LOCK;

    //只要线程池还开着，管理者线程就一直执行
    while (pool->state == THREAD_POOL_STATE_OPEN)
    {
        int add_cnt,desc_cnt;


        if((add_cnt = is_add_worker(pool))>0){
            //如果要增加线程
            for (int i = 0; i < add_cnt; i++)
            {
                threadpool_create_worker(pool);
            }
            log_message(LOGLEVEL_INFO,"manager add %d worker",add_cnt);
            log_threadpool_status(pool);
        }else if((desc_cnt = is_desc_worker(pool))>0){
            //如果要减少线程
            threadpool_eliminate_workers(pool,desc_cnt);
            log_message(LOGLEVEL_INFO,"manager desc %d worker",desc_cnt);
            log_threadpool_status(pool);
        }


        THREADPOOL_UNLOCK;
        //thread_yield();
        printf("i am manager is working\r\n");
        sleep(2);
        THREADPOOL_LOCK;

    }
    THREADPOOL_UNLOCK;
    thread_exit();
    
}