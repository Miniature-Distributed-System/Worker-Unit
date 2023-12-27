#include <stdlib.h>
#include <pthread.h>
#include <cstring>
#include <unistd.h>
#include <bits/stdc++.h>
#include "sched.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../configs.hpp"
#include "../services/global_objects_manager.hpp"
#include "process_manager.hpp"
#include "task_pool.hpp"

bool schedulerShouldStop = 0;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<ThreadQueue*> *threadQueueList;

#define HIGH_PRIORITY_TASK_TIMESLICE_MUL 6
#define MEDIUM_PRIORITY_TASK_TIMESLICE_MUL 4
#define LOW_PRIORITY_TASK_TIMESLICE 2

int calculate_priority_timeslice(TaskPriority prior)
{   
    int rc;
    switch(prior)
    {
        case NON_PREEMTABLE: rc = 0;break;
        case HIGH_PRIORITY: rc = HIGH_PRIORITY_TASK_TIMESLICE_MUL * NANOSECS;break;
        case MEDIUM_PRIORITY: rc = MEDIUM_PRIORITY_TASK_TIMESLICE_MUL * NANOSECS;break;
        case LOW_PRIORITY: 
        default: rc = LOW_PRIORITY_TASK_TIMESLICE * NANOSECS;break;
    }
    return rc;
}

JobStatus QueueJob::runStartProcess()
{
    return processTable->processState->start_proc(processTable->args);
}

JobStatus QueueJob::runPauseProcess()
{
    return processTable->processState->pause_proc(processTable->args);
}

void QueueJob::runEndProcess()
{
    processTable->processState->end_proc(processTable->args);
}

void QueueJob::runFailProcess()
{ 
    processTable->processState->fail_proc(processTable->args); 
}
ThreadQueue::ThreadQueue(unsigned int threadID){
    this->threadID = threadID;
    sem_init(&threadResourceLock, 0, 1);
    threadShouldStop.initFlag(false);
    for(int i = 0; i < QUEUE_SIZE; i++) jobSlotDone[i].initFlag(true);
    totalJobsInQueue = head = 0;
}

int ThreadQueue::addNewTask(QueueJob* job)
{
    if(!job)
        return -1;

    for(int i = 0; i < QUEUE_SIZE; i++){
        if(jobSlotDone[i].isFlagSet()){
            jobSlotDone[i].resetFlag();
            jobQueue[i] = job;
            totalJobsInQueue++;
            Log().schedINFO(__func__, "job inserted at slot:", i, " total pending jobs:", totalJobsInQueue);
            return 0;
        }
    }

    return -1;
}

QueueJob* ThreadQueue::getNextTask()
{
    sem_wait(&threadResourceLock);
    for(int i = 0; i < QUEUE_SIZE; i++){
        head = ++head % QUEUE_SIZE;
        if(!jobSlotDone[head].isFlagSet()){
            sem_post(&threadResourceLock);
            return jobQueue[head];
        }
    }

    sem_post(&threadResourceLock);
    return NULL;
}

QueueJob* ThreadQueue::popJob()
{
    sem_wait(&threadResourceLock);
    for(int i = 0; i < QUEUE_SIZE; i++){
        if(!jobSlotDone[i].isFlagSet() && !jobQueue[i]->isJobStatusSet(JOB_RUNNING) && 
            !jobQueue[i]->isJobStatusSet(JOB_FINALIZED)){
                QueueJob *job = jobQueue[i];
                jobSlotDone[i].setFlag();
                totalJobsInQueue--;
                jobQueue[i] = NULL;
                sem_post(&threadResourceLock);
                return job;
        }
    }
    sem_post(&threadResourceLock);
    return NULL;
}

void ThreadQueue::markTaskAsComplete()
{
    jobQueue[head]->setJobStatus(JOB_DONE);
    jobSlotDone[head].setFlag();
    totalJobsInQueue--;
    Log().schedINFO(__func__, "job slot:", head, " marked as complete, jobs pending:", totalJobsInQueue);
}

void ThreadQueue::flushFinishedJobs()
{
    Log().schedINFO(__func__, "cleaning ThreadID:", threadID, " queue");
    for(int i = 0; i < QUEUE_SIZE; i++){
        if(jobSlotDone[i].isFlagSet()){
            if(!jobQueue[i])
                continue;
            delete jobQueue[i];
            jobQueue[i] = NULL;
        }
    }
}

int ThreadQueue::getTotalJobsInQueue() 
{
    return totalJobsInQueue;
}

int ThreadQueue::getTotalWaitTime()
{
    if(totalJobsInQueue == 0)
        return 0;

    int totalWaitTime = 0;

    for(int i = 0; i < QUEUE_SIZE; i++){
        if(!jobSlotDone[i].isFlagSet() && jobQueue[i]){
            totalWaitTime += jobQueue[i]->getCpuTimeSlice() ?  jobQueue[i]->getCpuTimeSlice() : 999;
        }
    }
    return totalWaitTime;
}

int ThreadQueue::getThreadId()
{
    return threadID;
}

bool ThreadQueue::shouldStop()
{
    return threadShouldStop.isFlagSet();
}

void* start_job_timer(void *data)
{
    struct JobTimer* jTimer = (struct JobTimer*)data;
    struct timespec tim;

    tim.tv_nsec = jTimer->allowedCpuSlice;
    tim.tv_sec = 0;
    nanosleep(&tim, NULL);
    if(jTimer)
        jTimer->jobShouldPause = 0;

    return 0;
}

struct JobTimer* init_timer(QueueJob* job)
{
    pthread_t timerThread;
    struct JobTimer* jTimer = new JobTimer;
    jTimer->jobShouldPause = 0;

    if(!job->getCpuTimeSlice()){ 
        return jTimer;   
    }

    jTimer->allowedCpuSlice = job->getCpuTimeSlice();
    pthread_create(&timerThread, NULL, start_job_timer, (void*)jTimer);
    Log().schedINFO(__func__,"job timer created with timer set for:",
        jTimer->allowedCpuSlice,"ns");

    return jTimer;
}

int get_total_empty_slots(void)
{
    int i, totalSlots = 0;

    for(i = 0; i < globalConfigs.getTotalThreadCount(); i++)
    {
        totalSlots += QUEUE_SIZE - threadQueueList->at(i)->getTotalJobsInQueue();
    }
    Log().schedINFO(__func__, "Total slots in queue:", std::to_string(totalSlots));
    return totalSlots;
}

int get_quickest_queue(void)
{
    struct ThreadQueue *queue;
    int threadID = -1;
    std::uint64_t totalWaitTime, lowestWaitTime = INT_MAX, waitTime;
    int i,j;

    for(i = 0; i < globalConfigs.getTotalThreadCount(); i++)
    {
        totalWaitTime = waitTime = 0;

        if(threadQueueList->at(i)->getTotalJobsInQueue() >= QUEUE_SIZE)
            continue;
        
        for(j = 0; j < globalConfigs.getTotalThreadCount(); j++)
        {
            totalWaitTime = threadQueueList->at(i)->getTotalWaitTime();
        }
        if(lowestWaitTime > totalWaitTime)
        {
            threadID = i;
            lowestWaitTime = totalWaitTime;
        }
    }
    Log().schedINFO(__func__, "Thread ID:", threadID, 
                    " Total wait time:", lowestWaitTime);
    return threadID;
}

QueueJob* init_job(TaskData pTable)
{
    QueueJob *job = new QueueJob(pTable.procTable);
    job->setCpuTimeSlice(calculate_priority_timeslice(pTable.procTable->priority) / pTable.procTable->processState->type);
    Log().schedINFO(__func__, "job inited with cts:", job->getCpuTimeSlice() + 0);
    return job;
}

int get_empty_thread_queue()
{
    for(int i = 0; i < globalConfigs.getTotalThreadCount(); i++){
        if(!threadQueueList->at(i)->getTotalWaitTime())
            return i;
    }

    return -1;
}

int get_busiest_thread_queue()
{
    int busiestThreadId = -1;
    int largestWaitTime = 0;

    for(int i = 0; i < globalConfigs.getTotalThreadCount(); i++){
        // Don't bother if there is only one job in queue
        if(threadQueueList->at(i)->getTotalJobsInQueue() < 2)
            continue;
        int tempWaitTime = threadQueueList->at(i)->getTotalWaitTime();
        if(tempWaitTime > largestWaitTime)
            busiestThreadId = i;
    }

    return busiestThreadId;
}

void rebalance_thread_queues()
{
    if(get_total_empty_slots() <= 0)
        return;

    // Loop until all threads are equavalently balanced
    for(int i = 0; i < globalConfigs.getTotalThreadCount() - 1; i++){
        // Find an idle thread id
        int freeThreadId = get_empty_thread_queue();
        if(freeThreadId < 0)
            break;  // No idle threads present no need to idle balance
        //Find thread id with the most filled queue
        int busiestThreadId = get_busiest_thread_queue();
        if(busiestThreadId == freeThreadId || busiestThreadId < 0)
            return; // This means the load is balanced and no more rebalancing is required
        Log().schedINFO(__func__, "rebalancing ThreadID:", freeThreadId, " with ThreadID:", busiestThreadId);
        // Pop any non running job from busiest thread onto the freest thread queue and balance them out
        while(threadQueueList->at(freeThreadId)->getTotalWaitTime() < 
            threadQueueList->at(busiestThreadId)->getTotalWaitTime()){
                if(threadQueueList->at(freeThreadId)->addNewTask(threadQueueList->at(busiestThreadId)->popJob()))
                    break;
        }        
    }   
}

void *sched_task(void *ptr)
{
    struct ThreadQueue* queue;
    TaskData taskData;
    QueueJob* job;
    int i, j, qSlots, threadId;

    while(!schedulerShouldStop)
    {
        qSlots = get_total_empty_slots();
        for(int i = 0; i < globalConfigs.getTotalThreadCount(); i++)
            threadQueueList->at(i)->flushFinishedJobs();
        for(j = 0; j < qSlots; j++)
        {
            if(globalObjectsManager.get<TaskPool>().getTaskSinkSize() > 0)
            {
                Log().schedINFO(__func__, "scheulding jobs...");
                threadId = get_quickest_queue();
                if(threadId < 0)
                    break;  // No more space on thread queue, try again later
                if(getScheduledTask(taskData))
                    break;  // unlikely to happen but if it does better safe than sorry
                threadQueueList->at(threadId)->addNewTask(init_job(taskData));
            } else break;
        }
        rebalance_thread_queues();
        pthread_cond_wait(&cond, &mutex);
    }
    return 0;
}

void *thread_task(void *ptr)
{
    struct ThreadQueue *queue = (ThreadQueue*)ptr;
    std::string threadID = std::to_string(queue->getThreadId());

    if(!queue){
        Log().schedERR(__func__, "Thread queue not initilized exiting");
        return 0;
    }
    Log().schedINFO(__func__, "ThreadID:", threadID, " started successfully");

    while(!queue->shouldStop())
    {
        QueueJob *job = queue->getNextTask();
        if(!job)
            continue;

        if(job->isJobStatusSet(JOB_DONE))
        {
            if(job->jobErrorHandle.isFlagSet())
                job->runFailProcess();
            else
                job->runEndProcess();

            globalObjectsManager.get<ProcessManager>().unregisterProcess(job->processTable);
            queue->markTaskAsComplete();
            //signal scheduler to wake up
            pthread_cond_signal(&cond);
            continue;
        }

        job->setJobStatus(JOB_RUNNING);
        struct JobTimer *timer = init_timer(job);
        while(!timer->jobShouldPause)
        {
            job->setJobStatus(job->runStartProcess());
            
            if(job->isJobStatusSet(JOB_PENDING) && !(job->isJobStatusSet(JOB_DONE))){
                if(timer->jobShouldPause && job->getCpuTimeSlice())
                    job->runPauseProcess();
                else continue; // We are allowed to do another cycle of task
            } else if (job->isJobStatusSet(JOB_FAILED)){
                job->jobErrorHandle.setFlag();
            }
            break; // JOB_DONE signal is received or timeout or failure
        }
        delete timer;
    }

    return 0;
}

int init_sched(std::uint8_t max_thread)
{
    pthread_t sched_thread, *task_thread;
    int i,j, ret, pid;

    threadQueueList = new std::vector<ThreadQueue*>;
    for(i = 0; i < globalConfigs.getTotalThreadCount(); i++)
    {
        task_thread = new pthread_t;
        ThreadQueue *queue = new ThreadQueue(i);
        threadQueueList->push_back(queue);
        Log().schedINFO(__func__, "thread queue:", i, " inited successfully");
        pthread_create(task_thread, NULL, thread_task, threadQueueList->at(i));
    }
    pthread_create(&sched_thread, NULL, sched_task, NULL);

    return 0;
}

void exit_sched(void)
{
    struct ThreadQueue *queue;
    int i;

    //wait for already queued tasks to complete and empty
    while(get_total_empty_slots() - (globalConfigs.getTotalThreadCount() * QUEUE_SIZE)){
        sleep(2);
    }
    schedulerShouldStop = 1;
}
