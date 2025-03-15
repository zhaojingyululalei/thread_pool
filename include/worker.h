#ifndef __WORKER_H
#define __WORKER_H
#include "plat.h"
#include "list.h"
#include "rb_tree.h"
typedef enum {
    WORKER_STATE_NONE,
    WORKER_STATE_IDLE,  //该线程未持有任务
    WORKER_STATE_BUSY,  //该线程持有任务
    WORKER_STATE_CANCLE, //该任务被取消了
}worker_state_t; //工作线程状态


typedef struct {
    tid_t tid;
    worker_state_t state;
    bool exsit_done; // 该线程 是否已经完全退出
    list_node_t lnode; //用于调度
    rb_node_t rbnode; //用于快速查找
}worker_t;

int worker_init(void);
worker_t* worker_alloc(void);

void worker_free(worker_t* worker);

int worker_compare(const void *a, const void *b);
int worker_compare_by_key(const void* key,const void* b);
rb_node_t *worker_get_node(const void *data);
worker_t *node_get_worker(rb_node_t *node);
//debug
const char* get_worker_state_str(worker_state_t state);
void log_worker_list(const char* list_name, list_t* worker_list);
void debug_worker(worker_t* worker);
#endif