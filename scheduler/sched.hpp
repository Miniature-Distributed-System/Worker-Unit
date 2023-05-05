#ifndef SCHED_H
#define SCHED_H
#include <semaphore.h>
#include "../include/task.hpp"
#include "thread_pool.hpp"

#define QUEUE_SIZE 4
#define MAX_THREAD 10
#define QUEUE_ISFREE 1
#define QUEUE_FULL 0
#define NANOSECS 1000000
#define NS_TO_MS (a)  a * 1000000

extern pthread_cond_t  cond;
extern pthread_mutex_t mutex;

struct JobTimer {
    std::uint64_t allowedCpuSlice;
    bool jobShouldPause;
};

struct QueueJob {
    struct ProcessStates *proc;
    void *args;
    std::uint64_t cpuSliceMs;
    JobStatus jobStatus;
    bool jobErrorHandle;
    QueueJob(struct ProcessStates* proc, void* args){
        this->proc = proc;
        this->args = args;
    }
};

struct ThreadQueue {
    std::uint8_t threadID;
    sem_t threadResource;
    struct QueueJob *queueHead[QUEUE_SIZE];
    bool qSlotDone[QUEUE_SIZE];
    bool threadShouldStop;
    std::uint8_t totalJobsInQueue;
};

extern struct ThreadQueue *list[MAX_THREAD];
extern std::uint8_t allocatedThreads;
extern bool schedulerShouldStop;
int init_sched(struct ThreadPool *, std::uint8_t);
void exit_sched(void);
#endif
