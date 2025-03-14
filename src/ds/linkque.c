#include "linkque.h"

/**
 * 初始化链队
 */
void linkque_init(linkque_t *que) {
    que->head = NULL;
    que->tail = NULL;
    que->count = 0;
}

/**
 * 判断队列是否为空
 */
int linkque_is_empty(linkque_t *que) {
    return que->count == 0;
}

/**
 * 入队（使用已分配的 `linkque_node_t`）
 */
int linkque_enqueue(linkque_t *que, linkque_node_t *node) {
    if (!node) return 0;

    node->next = NULL;

    if (linkque_is_empty(que)) {
        que->head = que->tail = node;
    } else {
        que->tail->next = node;
        que->tail = node;
    }

    que->count++;
    return 1;
}

/**
 * 出队
 */
linkque_node_t* linkque_dequeue(linkque_t *que) {
    if (linkque_is_empty(que)) return NULL;

    linkque_node_t *remove_node = que->head;
    que->head = remove_node->next;

    if (!que->head) que->tail = NULL;  // 队列变空

    que->count--;
    return remove_node;
}