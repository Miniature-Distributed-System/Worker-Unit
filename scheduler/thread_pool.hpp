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

struct thread_pool_node {
    struct TaskData *pData;
    struct thread_pool_node *next;
};

struct thread_pool {
    struct thread_pool_node *headNode;
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

struct thread_pool* init_thread_pool();
void exit_thread_pool(struct thread_pool*);
TaskData thread_pool_pop(struct thread_pool*);
#endif