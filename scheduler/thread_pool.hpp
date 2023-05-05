#ifndef THREAD_H
#define THREAD_H
#include <semaphore.h>
#include <cstdint>

#include "../include/process.hpp"

//change this later lol
#define MAX_POOL_SIZE 10
#define PRIOR_1_STRVLMT 4
#define PRIOR_2_STRVLMT 6
#define PRIOR_3_STRVLMT 8

struct ThreadPoolNode {
    struct TaskData *pData;
    struct ThreadPoolNode *next;
};

struct ThreadPool {
    struct ThreadPoolNode *headNode;
    std::uint8_t threadPoolCount;
    sem_t threadPool_mutex;
};

class ThreadStructExport {
        TaskData *thread; 
    public:
        explicit ThreadStructExport(TaskData *thread = NULL) { this->thread = thread;}
        ~ThreadStructExport(){ delete thread; }
        TaskData& operator*(){ return *thread; } 
};

ThreadPool* init_thread_pool();
void exit_thread_pool(struct ThreadPool*);
TaskData thread_pool_pop(struct ThreadPool*);
#endif