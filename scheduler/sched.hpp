#ifndef SCHED_H
#define SCHED_H
#include <semaphore.h>
#include <vector>
#include "../include/task.hpp"
#include "thread_pool.hpp"
#include "../include/flag.h"

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

enum TaskExecutionStatus {
    RUNNING,
    WAITING,
    DONE
};

struct QueueJob {
    struct ProcessStates *proc;
    void *args;
    std::uint64_t cpuSliceMs;
    JobStatus jobStatus;
    TaskExecutionStatus taskStatus;
    bool jobErrorHandle;
    QueueJob(struct ProcessStates* proc, void* args){
        this->proc = proc;
        this->args = args;
        taskStatus = WAITING;
    }
};

class ThreadQueue {
        unsigned int threadID;
        int head;
        sem_t threadResourceLock;
        struct QueueJob *jobQueue[QUEUE_SIZE];
        Flag jobSlotDone[QUEUE_SIZE];
        Flag threadShouldStop;
        int totalJobsInQueue;
    public:
        ThreadQueue(){}
        ThreadQueue(unsigned int threadID);
        struct QueueJob* getNextTask();
        struct QueueJob* popJob();
        void markTaskAsPreempted();
        void markTaskAsComplete();
        int addNewTask(struct QueueJob *job);
        void flushFinishedJobs();
        int getTotalJobsInQueue();
        bool shouldStop();
        int getTotalWaitTime();
        int getThreadId();
};

extern std::vector<ThreadQueue*> *threadQueueList;
extern bool schedulerShouldStop;
int init_sched(struct ThreadPool *, std::uint8_t);
void exit_sched(void);
#endif
