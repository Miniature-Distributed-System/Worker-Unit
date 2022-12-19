#ifndef THREAD_H_
#define THREAD_H_
#include <semaphore.h>

//change this later lol
#define MAX_POOL_SIZE 10
#define PRIOR_1_STRVLMT 4
#define PRIOR_2_STRVLMT 6
#define PRIOR_3_STRVLMT 8

struct process {
    int (*start_proc)(void*);
    int (*pause_proc)(void*);
    int (*end_proc)(void*);
};

struct process_table {
    struct process *proc;
    void *args;
    unsigned int priority;
    unsigned int starveCounter;
};

struct thread_pool_node {
    struct process_table *pData;
    struct thread_pool_node *next;
};

extern struct thread_pool *threadPoolHead;
extern unsigned int threadPoolCount;
extern sem_t threadPool_mutex;
struct thread_pool {
    struct thread_pool_node *headNode;
    uint8_t threadPoolCount;
    sem_t threadPool_mutex;
};

int init_thread_pool();
void exit_thread_pool();
int sched_task(struct process* , void *, int);
#endif