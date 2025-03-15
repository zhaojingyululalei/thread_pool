#include "worker.h"
#include "threadpoolcfg.h"
#include "debug.h"
#include "log.h"
static worker_t workers[THREAD_POOL_MAX_SIZE];
static list_t worker_free_list;

int worker_compare(const void *a, const void *b)
{
    tid_t a_key = ((worker_t *)a)->tid;
    tid_t b_key = ((worker_t *)b)->tid;
    
    return  a_key-b_key ;
}

rb_node_t *worker_get_node(const void *data)
{
    return &((worker_t *)data)->rbnode;
}

worker_t *node_get_worker(rb_node_t *node)
{
    worker_t* w = rb_node_parent(node, worker_t, rbnode);
    return w;
}
int worker_init(void){
    list_init(&worker_free_list);
    for (int i = 0; i < THREAD_POOL_MAX_SIZE; i++)
    {
        list_insert_last(&worker_free_list,&workers[i].lnode);
    }
    
}
worker_t* worker_alloc(void){
    if(!list_count(&worker_free_list)){
        dbg_error("no more worker_t in worker_free_list\r\n");
        return NULL;
    }
    list_node_t * node = list_remove_first(&worker_free_list);
    worker_t* worker = list_node_parent(node,worker_t,lnode);
    memset(worker,0,sizeof(worker_t));
    return worker;
}

void worker_free(worker_t* worker){
    memset(worker,0,sizeof(worker_t));
    list_insert_last(&worker_free_list,&worker->lnode);
}

// 获取 worker 状态的字符串表示
const char* get_worker_state_str(worker_state_t state) {
    switch (state) {
        case WORKER_STATE_IDLE:       return "IDLE";
        case WORKER_STATE_BUSY:       return "BUSY";
        case WORKER_STATE_CANCLE: return "CANCELLING";
        default:                return "UNKNOWN";
    }
}

// 打印 worker_t 结构体信息
void log_worker_list(const char* list_name, list_t* worker_list) {
    log_message(LOGLEVEL_DEBUG, "[%s]: Total Workers: %d", list_name, list_count(worker_list));

    list_node_t* node = list_first(worker_list);
    while (node) {
        worker_t* worker = list_node_parent(node, worker_t, lnode);
        log_message(LOGLEVEL_DEBUG, "[%s]: Worker base:0x%x,Worker TID: %lu, State: %s",
                    list_name, (uintptr_t)worker,worker->tid, get_worker_state_str(worker->state));

        node = list_node_next(node);
    }
}
