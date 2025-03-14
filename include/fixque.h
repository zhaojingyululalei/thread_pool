#ifndef __FIXQUE_H
#define __FIXQUE_H

#include "ds_comm.h"
// 固定大小的环形队列结构体
typedef struct _fixque_t {
    int head;          // 头部索引（出队）
    int tail;          // 尾部索引（入队）
    int capacity;      // 队列最大存储容量
    int count;         // 当前元素数量
    void **buffer;     // 存放指针的数组

    
} fixque_t;

// 队列 API
void fixque_init(fixque_t *que, void **buffer, int capacity);
int fixque_is_empty(fixque_t *que);
int fixque_is_full(fixque_t *que);
int fixque_enqueue(fixque_t *que, void *value);
int fixque_dequeue(fixque_t *que, void **value);

#endif