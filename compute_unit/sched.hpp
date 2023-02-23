#ifndef SCHED_H
#define SCHED_H
#include <semaphore.h>
#include "thread_pool.hpp"

#define QUEUE_SIZE 4
#define MAX_THREAD 2
#define QUEUE_ISFREE 1
#define QUEUE_FULL 0
#define NANOSECS 1000000
#define NS_TO_MS (a)  a * 1000000

struct job_timer {
    uint64_t allowed_cpu_slice;
    std::uint64_t allowedCpuSlice;
    bool jobShouldPause;
};

struct queue_job {
    struct process *proc;
    void *args;
    uint64_t cpu_slice_ns; 
    bool jobDone;
    std::uint64_t cpuSliceMs;
    bool jobFinishPending;
    bool jobErrorHandle;
    queue_job(struct process* proc, void* args){
        this->proc = proc;
        this->args = args;
    }
};

struct thread_queue {
    std::uint8_t threadID;
    sem_t threadResource;
    struct queue_job *queueHead[QUEUE_SIZE];
    bool qSlotDone[QUEUE_SIZE];
    bool needsReorder;
    bool thread_should_stop;
    uint8_t headPointer;
    uint8_t tailPointer;
    uint8_t totalJobsInQueue;
    std::uint8_t totalJobsInQueue;
};

extern struct thread_queue *list[MAX_THREAD];
extern std::uint8_t allocatedThreads;
extern bool sched_should_stop;
int init_sched(struct thread_pool *, std::uint8_t);
void exit_sched(void);
#endif