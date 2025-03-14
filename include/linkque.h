#ifndef __LINKQUE_H
#define __LINKQUE_H
#include "ds_comm.h"

#define linkque_node_parent(node, parent_type, node_name)   \
        ((parent_type *)(node ? offset_to_parent((node), parent_type, node_name) : 0))
// 链表节点
typedef struct _linkque_node_t {
    struct _linkque_node_t *next; // 指向下一个节点
} linkque_node_t;
// 链队列结构
typedef struct _linkque_t {
    linkque_node_t *head;         // 头节点（出队）
    linkque_node_t *tail;         // 尾节点（入队）
    int count;                    // 队列大小

} linkque_t;

// 队列操作
void linkque_init(linkque_t *que);
int linkque_is_empty(linkque_t *que);
int linkque_enqueue(linkque_t *que, linkque_node_t *node);
linkque_node_t* linkque_dequeue(linkque_t *que);
#endif