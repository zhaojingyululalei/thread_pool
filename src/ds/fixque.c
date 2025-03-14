#include <string.h>
#include <stdint.h>
#include "fixque.h"
/**
 * 初始化队列
 * @param que 队列指针
 * @param buffer 用户提供的存储指针数组
 * @param capacity 队列容量
 * @param assign_callback 可选的深拷贝回调
 */
void fixque_init(fixque_t *que, void **buffer, int capacity)
{
    que->head = 0;
    que->tail = 0;
    que->capacity = capacity;
    que->count = 0;
    que->buffer = buffer;
}

/**
 * 判断队列是否为空
 */
int fixque_is_empty(fixque_t *que)
{
    return que->count == 0;
}

/**
 * 判断队列是否已满
 */
int fixque_is_full(fixque_t *que)
{
    return que->count == que->capacity;
}

/**
 * 入队操作（存储指针）
 */
int fixque_enqueue(fixque_t *que, void *value)
{
    if (fixque_is_full(que))
    {
        return 0; // 队列满，无法入队
    }

    que->buffer[que->tail] = value; // 直接存储指针

    que->tail = (que->tail + 1) % que->capacity; // 更新尾部索引
    que->count++;
    return 1;
}

/**
 * 出队操作（取出指针）
 */
int fixque_dequeue(fixque_t *que, void **value)
{
    if (fixque_is_empty(que))
    {
        return 0; // 队列为空
    }

    *value = que->buffer[que->head]; // 取出存储的指针

    que->head = (que->head + 1) % que->capacity; // 更新头部索引
    que->count--;
    return 1;
}