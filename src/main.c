#include "threadpool.h"

#include "debug.h"
threadpool_t pool;
static void* task_buf[MAX_JOB_CNT];


static void* calc(void* arg){
    int x = (int)arg;
    sleep(5);
    printf(" i am job calc,arg:%d\r\n",x);
    return 2*x;
}
void threadpool_test(void){
    int job_cnt = 20;
    job_t* arr[20] ={0};
    for (int i = 0; i < job_cnt; i++)
    {
        arr[i] = threadpool_submit_job(&pool,JOB_TYPE_ONCE,calc,i,true);
        
    }
    for (int i = 0; i < job_cnt; i++)
    {
        int ret = (int)get_job_ret(&pool,arr[i]);
        printf("********************************get job ret = %d\r\n",ret);
    }
    
    
    //printf("hello\r\n");
}
int main(int argc, char const *argv[])
{
    int ret;
    ret = threadpool_init(&pool,task_buf,MAX_JOB_CNT);
    if(ret <0){
        dbg_info("threadpool init fail\r\n");
        return 0;
    }
    
    threadpool_test();
    //pool_debug_all_list(&pool);
    sleep(5);
    threadpool_test();
    //pool_debug_all_list(&pool);
    printf("wait for collect child\r\n");
    while (true)
    {
        sleep(5);
    }
    

    return 0;
}
