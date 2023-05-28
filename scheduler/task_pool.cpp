#include <unistd.h>
#include "../include/packet.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../sender_proc/sender.hpp"
#include "../services/stats_engine.hpp"
#include "sched.hpp"
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


int TaskPool::pushTask(TaskData taskData)
{
    sem_wait(&sinkLock);
    if(taskSink.size() == 0){
        taskSink.push_back(taskData);
    }
    for(auto i = taskSink.begin(); i != taskSink.end(); i++)
    {
        TaskData iteratedData = *i;
        if(iteratedData.priority < taskData.priority){
            for(auto j = i; j != taskSink.end(); j++){
                // Task priority is promoted once its starved in its priority level
                if((*j).starveCounter > get_starve_limit((*j).starveCounter)){
                    (*j).priority - 1;
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
    sem_post(&sinkLock);
    return 0;
}

int TaskPool::popTask(TaskData &taskData)
{
    if(!taskSink.size())
        return -1;
    sem_wait(&sinkLock);
    taskData = taskSink.front();
    taskSink.pop_front();
    sem_post(&sinkLock);
    return 0;
}

/* This function initilises the process table for the submitted
 * process along with null checks. Once inited the process table
 * is pushed into the thread_pool.
 * In case of failure it returns EXIT_FAILURE macro else 0.
 */
int scheduleTask(ProcessStates *newProc, void *args, TaskPriority prior)
{
    TaskData newTask;
    
    if(newProc == NULL || args == NULL){
        return EXIT_FAILURE;
    }
    if(newProc->start_proc == NULL || newProc->end_proc == NULL)
        return EXIT_FAILURE;

    newTask.proc = newProc;
    newTask.args = args;
    newTask.priority = prior;
    newTask.starveCounter = 0;
    
    taskPool.pushTask(newTask);
    Log().taskPoolInfo(__func__,"insert node:", taskPool.getTaskSinkSize());
    pthread_cond_signal(&cond);

    return 0;
}

int getScheduledTask(TaskData &taskData)
{
    if(taskPool.popTask(taskData)){
        Log().info(__func__, "Task pool is empty");
        return -1;
    }
    return 0;
}
