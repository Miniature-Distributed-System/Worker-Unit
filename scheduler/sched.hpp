#ifndef SCHED_H
#define SCHED_H
#include <semaphore.h>
#include <vector>
#include "../include/task.hpp"
#include "../include/flag.h"
#include "../include/process.hpp"

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

class QueueJob {
        std::uint64_t cpuSliceMs;
    public:
        ProcessTable *processTable;
        Flag jobErrorHandle;
        QueueJob(ProcessTable *processTable){
            this->processTable = processTable;
            cpuSliceMs = 0;
        }
        void setCpuTimeSlice(std::uint64_t cpuTimeSlice) { this->cpuSliceMs = cpuSliceMs; }
        std::uint64_t getCpuTimeSlice() { return cpuSliceMs; } 
        JobStatus runStartProcess();
        JobStatus runPauseProcess();
        void runEndProcess();
        void runFailProcess();
        void setJobStatus(JobStatus status) { processTable->jobStatus = processTable->jobStatus | status; }
        void resetJobStatus(JobStatus status) { processTable->jobStatus &= ~(1 << status); }
        bool isJobStatusSet(JobStatus status) { if(status & processTable->jobStatus) return true; return false; }
};

class ThreadQueue {
        unsigned int threadID;
        int head;
        sem_t threadResourceLock;
        QueueJob *jobQueue[QUEUE_SIZE];
        Flag jobSlotDone[QUEUE_SIZE];
        Flag threadShouldStop;
        int totalJobsInQueue;
    public:
        ThreadQueue(){}
        ThreadQueue(unsigned int threadID);
        QueueJob* getNextTask();
        QueueJob* popJob();
        void markTaskAsComplete();
        int addNewTask(QueueJob *job);
        void flushFinishedJobs();
        int getTotalJobsInQueue();
        bool shouldStop();
        int getTotalWaitTime();
        int getThreadId();
};

extern std::vector<ThreadQueue*> *threadQueueList;
extern bool schedulerShouldStop;
int init_sched(std::uint8_t);
void exit_sched(void);
#endif
