#ifndef THREAD_H
#define THREAD_H
#include <semaphore.h>
#include <cstdint>
#include <list>

#include "../include/process.hpp"
#include "../include/flag.h"

//change this later lol
#define MAX_POOL_SIZE 10
#define PRIOR_1_STRVLMT 4
#define PRIOR_2_STRVLMT 6
#define PRIOR_3_STRVLMT 8

struct TaskData {
    ProcessTable *procTable;
    TaskPriority priority;
    std::uint16_t starveCounter;
    TaskData(){
        procTable = NULL;
    };
};

class TaskPool {
        std::list<TaskData> taskSink;
        sem_t sinkLock;
    public:
        TaskPool();
        int pushTask(TaskData procTable);
        int popTask(TaskData &taskData);
        int getTaskSinkSize() { return taskSink.size(); }
};

extern TaskPool taskPool;
void exit_thread_pool(struct ThreadPool*);
int getScheduledTask(TaskData &taskData);
int reScheduleTask(ProcessTable *procTable);
#endif