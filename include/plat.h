#ifndef __PLAT_H
#define __PLAT_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <semaphore.h>
#include <string.h>

typedef pthread_t tid_t;

typedef void*(*thread_routine)(void*);

// 互斥锁封装
typedef pthread_mutex_t lock_t;

// 信号量封装
typedef sem_t semaphore_t;

// 线程 API
tid_t thread_create(thread_routine func, void* arg);
void thread_join(tid_t thread);
void thread_detach(tid_t thread);
void thread_exit(void);

// 互斥锁 API
void lock_init(lock_t* mutex);
void lock_destroy(lock_t* mutex);
void lock(lock_t* mutex);
void unlock(lock_t* mutex);

// 信号量 API
void semaphore_init(semaphore_t* sem, unsigned int value);
void semaphore_destroy(semaphore_t* sem);
void semaphore_wait(semaphore_t* sem);
void semaphore_post(semaphore_t* sem);

tid_t gettid(void);
#endif