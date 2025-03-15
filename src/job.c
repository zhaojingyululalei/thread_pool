#include "threadpool.h"
#include "debug.h"
#include "job.h"
lock_t job_lock;
static job_t jobs[MAX_JOB_CNT];
static  list_t job_free_list; //存放未分配的jobt结构
 list_t job_list; //存放分配出的jobt结构
job_t* job_buffer[MAX_JOB_CNT];
int job_init(void){

    lock_init(&job_lock);
    //初始化分配队列
    list_init(&job_free_list);
    list_init(&job_list);
    for (int i = 0; i < MAX_JOB_CNT; i++)
    {
        list_insert_last(&job_free_list,&jobs[i].node);
    }
    
}

job_t* job_alloc(void)
{
    if(!list_count(&job_free_list)){
        dbg_error("no more job_t in job_free_list\r\n");
        return NULL;
    }
    list_node_t * node = list_remove_first(&job_free_list);
    job_t* job = list_node_parent(node,job_t,node);
    memset(job,0,sizeof(job_t));

    list_insert_last(&job_list,&job->node);
    return job;
}

void job_free(job_t* job){
    list_remove(&job_list,&job->node);
    memset(job,0,sizeof(job_t));
    list_insert_last(&job_free_list,&job->node);
}

void* get_job_ret(job_t* job){
    void* ret;
    lock(&job_lock);
    if(job->is_needret){
        //如果用户需要读取返回值，那么就等着任务结束,并手动设置job.isdone
        unlock(&job_lock);
        semaphore_wait(&job->ret_sem);
        lock(&job_lock);
    }else{
        //用户不需要返回值的话，工作线程自动设置为 job.isdone
        unlock(&job_lock);
        return NULL;
    }
    
    
    
    ret = job->ret;
    job->is_readed = true;
    job->is_done = true;//标记这个job_t可以回收掉了
    unlock(&job_lock);
    return ret;
}