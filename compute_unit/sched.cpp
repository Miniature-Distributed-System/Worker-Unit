#include <stdlib.h>
#include <pthread.h>
#include <cstring>
#include <unistd.h>
#include "sched.hpp"
#include "thread_pool.hpp"
#include "receiver_proc/debug_rp.hpp"
//this var needs refactor should make it local scope
struct thread_queue *list[MAX_THREAD];
bool sched_should_stop = 0;
int get_cpu_slice(int prior)
{   
    int rc;
    switch(prior)
    {
        case 0: rc = 0;break;
        case 1: rc = 6 * NANOSECS;break;
        case 2: rc = 4 * NANOSECS;break;
        case 3: rc = 2 * NANOSECS;break;
    }
    return rc;
}
{
    }

int get_total_empty_slots(void)
{
    struct thread_queue* queue;
    int i, j, totalSlots = 0;

    for(i = 0; i < MAX_THREAD; i++)
    {
        queue = list[i];
        for(j = 0; j < QUEUE_SIZE; j++)
        {
            if(queue->qSlotDone[j])
                totalSlots++;
        }
    }
    DEBUG_MSG(__func__, "Total slots in queue:", totalSlots);
    return totalSlots;
}

struct thread_queue* get_quickest_queue(void)
{
    struct thread_queue *queue;
    int threadID = 0, totalWaitTime, lowestWaitTime = INT_MAX, waitTime;
    int i,j;

    for(i = 0; i < MAX_THREAD; i++)
    {
        queue = list[i];
        totalWaitTime = waitTime = 0;
        if(queue->totalJobsInQueue == QUEUE_SIZE)
            continue;
        
        for(j = 0; j < MAX_THREAD; j++)
        {
            if(!queue->qSlotDone[i])
            {
                waitTime = queue->queueHead[i]->cpuSliceMs;
                totalWaitTime += waitTime ? waitTime : 999;
            }
        }
        if(lowestWaitTime > totalWaitTime)
        {
            lowestWaitTime = totalWaitTime;
            threadID = i;
        }
    }
    DEBUG_MSG(__func__, "Thread ID:", threadID, 
                    " Total wait time:", lowestWaitTime);
    return list[threadID];
}

struct queue_job* init_job(struct process_table* pTable)
{
    struct queue_job *job = new queue_job;
    job->args = pTable->args;
    job->proc = pTable->proc;
    job->jobFinishPending = job->jobErrorHandle = 0;
    job->cpuSliceMs = get_cpu_slice(pTable->priority);
    DEBUG_MSG(__func__, "job inited with cts:",job->cpuSliceMs);
    return job;
}

void dealloc_job(struct queue_job* job)
{
    delete job->args;
    delete job->proc;
    delete job;
}

void *sched_task(void *ptr)
{
    struct thread_pool* threadPoolHead = (struct thread_pool*)ptr;
    struct thread_queue* queue;
    struct process_table* proc;
    struct queue_job* job;
    int i, j, qSlots;

    if(threadPoolHead == NULL){
        DEBUG_ERR(__func__, "thread head is uninited cant procced any further!");
        return;
    }

    while(!sched_should_stop)
    {
        qSlots = get_total_empty_slots();
        for(j = 0; j < qSlots; j++)
        {
            if(threadPoolHead->threadPoolCount > 0)
            {
                DEBUG_MSG(__func__, "scheulding jobs...");
                queue = get_quickest_queue();
                for(i = 0; i < QUEUE_SIZE; i++)
                {
                    //insert into first free slot
                    if(queue->qSlotDone[i])
                    {
                        //first release memory
                        if(queue->queueHead[i])
                            dealloc_job(queue->queueHead[i]);
                        
                        proc = thread_pool_pop(threadPoolHead);
                        job = init_job(proc);
                        queue->queueHead[i] = job;
                        queue->totalJobsInQueue++;
                        DEBUG_MSG(__func__, "job inserted at slot:", j,
                                "total pending jobs:", queue->totalJobsInQueue);
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

void *thread_task(void *ptr)
{
    struct thread_queue *queue = (thread_queue*)ptr;
    struct queue_job *job;
    struct job_timer *timer;
    int head = 0, ret;
    bool done;

    while(!queue->thread_should_stop)
    {
    }
}

int init_sched(struct thread_pool *thread)
{
    pthread_t sched_thread, *task_thread;
    struct thread_queue *queue;
    struct thread_pool* threadPoolHead;
    int i,j, ret, pid;

    for(i = 0; i < MAX_THREAD; i++)
    {
        task_thread = new pthread_t;
        queue = new thread_queue;
        if(queue == NULL){
            DEBUG_ERR(__func__,"queue alloc failed");
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
        DEBUG_MSG(__func__, "thread queue:", i, " inited successfully");
        pthread_create(task_thread, NULL, thread_task, (void*)queue);
    }
    pthread_create(&sched_thread, NULL, sched_task, NULL);

    return 0;
}

void exit_sched(void)
{
    struct thread_queue *queue;
    int i;
    
    for(i = 0; i < MAX_THREAD; i++){
        queue = list[i];
        if(queue != NULL){
            sem_destroy(&queue->threadResource);
            queue->thread_should_stop = 1;
        }
        sched_should_stop = 1;
    }
}