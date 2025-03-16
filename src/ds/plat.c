#include "plat.h"

// 线程创建
tid_t thread_create(thread_routine func, void* arg) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, func, arg) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    return thread;
}

// 线程等待
void thread_join(tid_t thread) {
    if (pthread_join(thread, NULL) != 0) {
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
    }
}



// 线程退出
void thread_exit(void) {
    pthread_exit(NULL);
}

//让出cpu
void thread_yield(void){
    sched_yield();
}

// 互斥锁初始化
void lock_init(lock_t* lock) {
    if (pthread_mutex_init(lock, NULL) != 0) {
        perror("pthread_lock_init failed");
        exit(EXIT_FAILURE);
    }
}

// 互斥锁销毁
void lock_destroy(lock_t* lock) {
    if (pthread_mutex_destroy(lock) != 0) {
        perror("pthread_lock_destroy failed");
        exit(EXIT_FAILURE);
    }
}

// 互斥锁加锁
void lock(lock_t* lock) {
    if (pthread_mutex_lock(lock) != 0) {
        perror("pthread_lock_lock failed");
        exit(EXIT_FAILURE);
    }
}

// 互斥锁解锁
void unlock(lock_t* lock) {
    if (pthread_mutex_unlock(lock) != 0) {
        perror("pthread_lock_unlock failed");
        exit(EXIT_FAILURE);
    }
}

// 信号量初始化
void semaphore_init(semaphore_t* sem, unsigned int value) {
    if (sem_init(sem, 0, value) != 0) {
        perror("sem_init failed");
        exit(EXIT_FAILURE);
    }
}

// 信号量销毁
void semaphore_destroy(semaphore_t* sem) {
    if (sem_destroy(sem) != 0) {
        perror("sem_destroy failed");
        exit(EXIT_FAILURE);
    }
}

// 信号量等待
void semaphore_wait(semaphore_t* sem) {
    if (sem_wait(sem) != 0) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

// 信号量释放
void semaphore_post(semaphore_t* sem) {
    if (sem_post(sem) != 0) {
        perror("sem_post failed");
        exit(EXIT_FAILURE);
    }
}

tid_t gettid(void){
    return  pthread_self();
}