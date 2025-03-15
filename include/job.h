#ifndef __JOB_H
#define __JOB_H
#include "threadpoolcfg.h"
#include "plat.h"
#include "list.h"
typedef enum {
    JOB_TYPE_NONE,
    JOB_TYPE_ONCE,
    JOB_TYPE_PERIOD,
}job_type_t;    //任务类型
// 任务结构体
typedef struct {
    job_type_t type;
    semaphore_t ret_sem;//想获取结果得等
    void* (*function)(void*); // 任务函数
    void* arg;              // 任务参数
    void* ret;  //任务返回值
    bool is_needret; //用户是否需要返回值
    bool is_readed; //如果需要返回值，用户是否已经读取了返回值
    bool is_done;//任务是否执行完毕
    list_node_t node;
} job_t;
extern job_t* job_buffer[MAX_JOB_CNT];
extern lock_t job_lock;
extern  list_t job_list;
int job_init(void);

job_t* job_alloc(void);

void job_free(job_t* job);
void* get_job_ret(job_t* job);

#endif