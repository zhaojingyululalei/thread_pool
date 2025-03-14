#ifndef __THREADPOOL_H
#define __THREADPOOL_H

#include "plat.h"
#include "fixque.h"
#include "list.h"
#include "threadpoolcfg.h"

typedef enum {
    JOB_TYPE_NONE,
    JOB_TYPE_ONCE,
    JOB_TYPE_PERIOD,
}job_type_t;    //任务类型
typedef enum {
    WORKER_STATE_NONE,
    WORKER_STATE_IDLE,  //该线程未持有任务
    WORKER_STATE_BUSY,  //该线程持有任务
    WORKER_STATE_CANCLE, //该任务被取消了
}worker_state_t; //工作线程状态
// 任务结构体
typedef struct {
    job_type_t type;
    semaphore_t ret_sem;//想获取结果得等
    void* (*function)(void*); // 任务函数
    void* arg;              // 任务参数
    void* ret;  //任务返回值
    bool is_readed; //返回值是否已经读取，读了就删了job_t这个结构了
} job_t;

typedef struct {
    tid_t tid;
    worker_state_t state;
    list_node_t node;
}worker_t;


typedef struct {
    enum{
        THREAD_POOL_STATE_CLOSE,
        THREAD_POOL_STATE_OPEN
    }state;
    lock_t lock;
    semaphore_t job_sem;                             // 没有任务就等，有任务去获取
    fixque_t jobq;                                   // 任务队列
    list_t idle_list;                            //没取到任务的线程队列
    list_t busy_list;                            //取到任务的线程队列
    list_t cancle_list;                            //将要退出的线程
    int thread_count;                                // 当前线程数
    int idle_count;                                  // 空闲线程数
    int busy_count;                                    // 线程池关闭标志
    tid_t manager;
} threadpool_t;

int threadpool_init(threadpool_t* pool,void** fixque_buf,int job_max_cnt);
int threadpool_create_worker(threadpool_t* pool);
worker_t* find_worker_by_tid(threadpool_t* pool,tid_t tid);
worker_state_t get_worker_state(threadpool_t* pool,worker_t* worker);
int get_threadpool_state(threadpool_t* pool);

worker_t *worker_alloc(void);
 void worker_free(worker_t *worker);

int worker_set_idle(threadpool_t* pool,worker_t* worker);
int worker_set_busy(threadpool_t* pool,worker_t* worker);
int worker_set_cancle(threadpool_t *pool, worker_t *worker);

job_t* threadpool_submit_job(threadpool_t* pool,job_type_t type,void* (*function)(void*),void* arg,bool need_ret);
int threadpool_eliminate_idle(threadpool_t *pool,int n);

void *get_job_ret(threadpool_t* pool,job_t* job);
void job_free(job_t *job);
job_t* get_job_by_idx(int idx);

void worker_debug_print(worker_t* worker);
void list_debug_print(list_t* list);
void pool_debug_all_list(threadpool_t* pool);
#endif