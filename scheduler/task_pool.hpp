#ifndef THREAD_H
#define THREAD_H
#include <semaphore.h>
#include <cstdint>
#include <list>

#include "../include/process.hpp"
#include "../include/flag.h"
#include "../services/global_objects_manager.hpp"

//change this later lol
#define MAX_POOL_SIZE 10
#define PRIOR_1_STRVLMT 4
#define PRIOR_2_STRVLMT 6
#define PRIOR_3_STRVLMT 8

struct TaskData {
    ProcessTable *procTable;
    TaskPriority poolPriority;
    std::uint16_t starveCounter;
    TaskData(){
        procTable = NULL;
    };
};

class TaskPool : public Base {
        std::list<TaskData> taskSink;
        sem_t sinkLock;
		static std::string classID;
    public:
        TaskPool();
		static const std::string& getId()
        {
            static const std::string id = "TaskPool";
            return id;
        }
        int pushTask(TaskData procTable);
        int popTask(TaskData &taskData);
        int getTaskSinkSize() { return taskSink.size(); }
};

void exit_thread_pool(struct ThreadPool*);
int getScheduledTask(TaskData &taskData);
int reScheduleTask(ProcessTable *procTable);
#endif
