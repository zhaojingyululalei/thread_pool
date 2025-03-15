
#include "threadpool.h"
void* pool_manager(void* arg){
    threadpool_t* pool = (threadpool_t*)arg;
    while (true)
    {
        sleep(100);
    }
    
}