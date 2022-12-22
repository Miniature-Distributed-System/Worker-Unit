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
{
    }

        }
    }
}

{
    }

}
void *sched_task(void *ptr)
{

    if(threadPoolHead == NULL){
        DEBUG_ERR(__func__, "thread head is uninited cant procced any further!");
        return;
    }

    while(!sched_should_stop)
    {
                }
            }
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