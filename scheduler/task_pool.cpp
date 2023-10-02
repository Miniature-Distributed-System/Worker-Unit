#include <unistd.h>
#include "../include/packet.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../sender_proc/sender.hpp"
#include "../services/stats_engine.hpp"
#include "sched.hpp"
#include "process_manager.hpp"
#include "task_pool.hpp"

/* This function returns how much the process of a certain 
 * priority can be starved. 
 */
int get_starve_limit(int prior)
{
    switch(prior){
        case 1: return PRIOR_1_STRVLMT;
        case 2: return PRIOR_2_STRVLMT;
        case 3: return PRIOR_3_STRVLMT;
        default: return 0;
    }
}

TaskPool::TaskPool()
{
    Log().info(__func__, "taskPool init");
    sem_init(&sinkLock, 0, 1);
}

int TaskPool::pushTask(TaskData taskData)
{
    sem_wait(&sinkLock);

    if(taskSink.size() == 0){
        taskSink.push_back(taskData);
    } else {
         for(auto i = taskSink.begin(); i != taskSink.end(); i++)
        {
            TaskData iteratedData = *i;
            if(iteratedData.poolPriority < taskData.poolPriority){
                for(auto j = i; j != taskSink.end(); j++){
                    // Task priority is promoted once its starved in its priority level
                    if((*j).starveCounter > get_starve_limit((*j).starveCounter)){
                        (*j).poolPriority - 1; // Raise task priority as a promotion award
                        (*j).starveCounter = 0;
                    } else {
                        (*j).starveCounter++;
                    }
                }
                taskSink.insert(i, taskData);
                sem_post(&sinkLock);
                return 0;
            }
        }
        taskSink.push_back(taskData);
    }
    sem_post(&sinkLock);
    return 0;
}

int TaskPool::popTask(TaskData &taskData)
{
    if(!taskSink.size())
        return EXIT_FAILURE;
    sem_wait(&sinkLock);
    taskData = taskSink.front();
    taskSink.pop_front();
    sem_post(&sinkLock);
    return 0;
}

bool isProcessTypeValid(ProcessType type)
{
    if(type < NR_PROCESS)
        return true;
    return false;
}

/* This function initilises the process table for the submitted
 * process along with null checks. Once inited the process table
 * is pushed into the thread_pool.
 * In case of failure it returns EXIT_FAILURE macro else 0.
 */
int scheduleTask(ProcessStates *newProc, void *args, TaskPriority prior)
{
    TaskData task;
    
    if(newProc == NULL){
        Log().error(__func__, "Process data is null!");
        return EXIT_FAILURE;
    }
    if(args == NULL){
         Log().error(__func__, "Arguments are null!");
        return EXIT_FAILURE;
    }
    if(newProc->start_proc == NULL || newProc->end_proc == NULL || !isProcessTypeValid(newProc->type)){
        Log().error(__func__, "Invalid ProcessStates structures");
        return EXIT_FAILURE;
    }

    task.procTable = globalProcessManager.registerProcess();
    taskPool.pushTask(task);
    Log().taskPoolInfo(__func__,"insert node:", taskPool.getTaskSinkSize());
    pthread_cond_signal(&cond);

    return 0;
}

int reScheduleTask(ProcessTable *procTable)
{
    TaskData task;
    task.procTable = procTable;

    taskPool.pushTask(task);
    Log().taskPoolInfo(__func__,"re-inserted node:", taskPool.getTaskSinkSize());
    pthread_cond_signal(&cond);

    return 0;
}

int getScheduledTask(TaskData &taskData)
{
    if(taskPool.popTask(taskData)){
        Log().taskPoolInfo(__func__, "Task pool is empty");
        return EXIT_FAILURE;
    }
    Log().taskPoolInfo(__func__, "Popped task from pool, size:", taskPool.getTaskSinkSize());
    return 0;
}

