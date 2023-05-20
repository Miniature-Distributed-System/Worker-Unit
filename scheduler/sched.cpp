#include <stdlib.h>
#include <pthread.h>
#include <cstring>
#include <unistd.h>
#include <bits/stdc++.h>
#include "sched.hpp"
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"

//this var needs refactor should make it local scope
std::uint8_t allocatedThreads;
struct ThreadQueue *list[MAX_THREAD];
bool schedulerShouldStop = 0;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define HIGH_PRIORITY_TASK_TIMESLICE_MUL 6
#define MEDIUM_PRIORITY_TASK_TIMESLICE_MUL 4
#define LOW_PRIORITY_TASK_TIMESLICE 2

int get_cpu_slice(TaskPriority prior)
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

struct JobTimer* init_timer(struct QueueJob* job)
{
    pthread_t timerThread;
    struct JobTimer* jTimer = new JobTimer;
    jTimer->jobShouldPause = 0;

    if(!job->cpuSliceMs){ 
        return jTimer;   
    }

    jTimer->allowedCpuSlice = job->cpuSliceMs;
    pthread_create(&timerThread, NULL, start_job_timer, (void*)jTimer);
    Log().schedINFO(__func__,"job timer created with timer set for:",
        jTimer->allowedCpuSlice,"ns");

    return jTimer;
}

int get_total_empty_slots(void)
{
    struct ThreadQueue* queue;
    int i, j, totalSlots = 0;

    for(i = 0; i < allocatedThreads; i++)
    {
        queue = list[i];
        for(j = 0; j < QUEUE_SIZE; j++)
        {
            if(queue->qSlotDone[j])
                totalSlots++;
        }
    }
    Log().schedINFO(__func__, "Total slots in queue:", std::to_string(totalSlots));
    return totalSlots;
}

struct ThreadQueue* get_quickest_queue(void)
{
    struct ThreadQueue *queue;
    int threadID = 0;
    std::uint64_t totalWaitTime, lowestWaitTime = INT_MAX, waitTime;
    int i,j;

    for(i = 0; i < allocatedThreads; i++)
    {
        queue = list[i];
        totalWaitTime = waitTime = 0;

        if(queue->totalJobsInQueue >= QUEUE_SIZE)
            continue;
        
        for(j = 0; j < allocatedThreads; j++)
        {
            if(!queue->qSlotDone[i])
            {
                waitTime = (int)queue->queueHead[i]->cpuSliceMs;
                totalWaitTime += waitTime ? waitTime : 999;
            }
        }
        if(lowestWaitTime > totalWaitTime)
        {
            threadID = i;
            lowestWaitTime = totalWaitTime;
        }
    }
    Log().schedINFO(__func__, "Thread ID:", threadID, 
                    " Total wait time:", lowestWaitTime);
    return list[threadID];
}

struct QueueJob* init_job(TaskData pTable)
{
    struct QueueJob *job = new QueueJob(pTable.proc, pTable.args);
    job->jobStatus = JOB_PENDING;
    job->jobErrorHandle = 0;
    job->cpuSliceMs = get_cpu_slice(pTable.priority);
    Log().schedINFO(__func__, "job inited with cts:", job->cpuSliceMs);
    return job;
}

void dealloc_job(struct QueueJob* job)
{
    delete job;
    Log().schedINFO(__func__, "deallocated job");
}

void *sched_task(void *ptr)
{
    struct ThreadPool* threadPoolHead = (struct ThreadPool*)ptr;
    struct ThreadQueue* queue;
    TaskData proc;
    struct QueueJob* job;
    int i, j, qSlots;

    if(threadPoolHead == NULL){
        Log().schedINFO(__func__, "thread head is uninited cant procced any further!");
        return 0;
    }

    while(!schedulerShouldStop)
    {
        qSlots = get_total_empty_slots();
        for(j = 0; j < qSlots; j++)
        {
            if(threadPoolHead->threadPoolCount > 0)
            {
                Log().schedINFO(__func__, "scheulding jobs...");
                queue = get_quickest_queue();
                for(i = 0; i < QUEUE_SIZE; i++)
                {
                    //insert into first free slot
                    if(queue->qSlotDone[i])
                    {
                        //first dealloc memory
                        if(queue->queueHead[i] && queue->qSlotDone[i])
                            dealloc_job(queue->queueHead[i]);
                        
                        proc = thread_pool_pop(threadPoolHead);
                        if(proc.args == NULL || proc.proc == NULL)
                            break;
                        job = init_job(proc);
                        queue->queueHead[i] = job;
                        queue->qSlotDone[i] = 0;
                        queue->totalJobsInQueue++;
                        Log().schedINFO(__func__, "job inserted at slot:", j,
                            " total pending jobs:", std::to_string(queue->totalJobsInQueue));
                        break;
                    }
                }
            }
        }
        pthread_cond_wait(&cond, &mutex);
    }
    return 0;
}

void *thread_task(void *ptr)
{
    struct ThreadQueue *queue = (ThreadQueue*)ptr;
    struct QueueJob *job;
    struct JobTimer *timer;
    int head = 0;
    bool done;
    std::string threadID = std::to_string(queue->threadID);

    if(!queue)
    {
        Log().schedERR(__func__, "Thread queue not initilized exiting");
        return 0;
    }
    Log().schedINFO(__func__, "ThreadID:", threadID, " started successfully");

    while(!queue->threadShouldStop)
    {
        if(!queue->qSlotDone[head] && queue->totalJobsInQueue)
        {
            job = queue->queueHead[head];
            timer = init_timer(job);
            Log().schedINFO(__func__, "ThreadID:", threadID, 
            " job slot in execution:", head);
            while(!timer->jobShouldPause)
            {
                if(job->jobStatus == JOB_DONE || job->jobStatus == JOB_FAILED)
                {
                    job->jobStatus = job->proc->end_proc(job->args, job->jobStatus);
                    queue->qSlotDone[head] = 1;
                    queue->totalJobsInQueue--;
                    //signal scheduler to wake up
                    pthread_cond_signal(&cond);  
                    break;
                }
                job->jobStatus = job->proc->start_proc(job->args);
                if(job->jobStatus == JOB_DONE){
                    Log().schedINFO(__func__, "ThreadID:", threadID, " Job done and awaiting to finish");
                    break;
                } else if (job->jobStatus == JOB_FAILED){
                    //must also do process error handling at the moment not implimented
                    Log().schedINFO(__func__, "ThreadID:", threadID, " Error encountered set error handling");
                    job->jobErrorHandle = 1;
                    break;
                }
            }
            if(job->proc->pause_proc && timer->jobShouldPause)
                job->proc->pause_proc(job->args);
        }
        head = ++head % QUEUE_SIZE;
    }

    return 0;
}

int init_sched(struct ThreadPool *thread, std::uint8_t max_thread)
{
    pthread_t sched_thread, *task_thread;
    struct ThreadQueue *queue;
    struct ThreadPool* threadPoolHead;
    int i,j, ret, pid;

    allocatedThreads = max_thread;
    for(i = 0; i < allocatedThreads; i++)
    {
        task_thread = new pthread_t;
        queue = new ThreadQueue;
        if(queue == NULL){
            Log().schedERR(__func__,"queue alloc failed");
            return EXIT_FAILURE;
        }

        sem_init(&queue->threadResource, 0 ,1);
        queue->threadShouldStop = 0;
        for(j = 0; j < QUEUE_SIZE; j++){
            queue->queueHead[j] = NULL;
            queue->qSlotDone[j] = 1;
        }
        queue->totalJobsInQueue = 0;
        queue->threadID = i;
        list[i] = queue;
        Log().schedINFO(__func__, "thread queue:", i, " inited successfully");
        pthread_create(task_thread, NULL, thread_task, (void*)queue);
    }
    pthread_create(&sched_thread, NULL, sched_task, (void*)thread);

    return 0;
}

void exit_sched(void)
{
    struct ThreadQueue *queue;
    int i;

    //wait for already queued tasks to complete and empty
    while(get_total_empty_slots() - (allocatedThreads * QUEUE_SIZE)){
        sleep(2);
    }

    for(i = 0; i < allocatedThreads; i++){
        queue = list[i];
        if(queue != NULL){
            queue->threadShouldStop = 1;
            sem_destroy(&queue->threadResource);
            delete queue;
        }
        schedulerShouldStop = 1;
    }
}
