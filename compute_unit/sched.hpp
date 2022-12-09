#ifndef SCHED_H
#define SCHED_H
#include <semaphore.h>
#include "thread_pool.hpp"

#define QUEUE_SIZE 4
#define MAX_THREAD 2
#define QUEUE_ISFREE 1
#define QUEUE_FULL 0
#define MILLISECS 1000000

struct job_timer {
    uint64_t allowed_cpu_slice;
    bool jobShouldPause;
};

struct queue_job {
    struct process *proc;
    void *args;
    uint64_t cpu_slice_ns; 
    bool jobDone;
};

struct thread_queue {
    sem_t threadResource;
    struct queue_job *queueHead[QUEUE_SIZE];
    bool qSlotDone[QUEUE_SIZE];
    bool needsReorder;
    bool thread_should_stop;
    uint8_t headPointer;
    uint8_t tailPointer;
    uint8_t totalJobsInQueue;
};

extern struct thread_queue *list[MAX_THREAD];
extern bool sched_should_stop;
#endif