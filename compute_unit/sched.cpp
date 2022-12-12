/* @function: reorder_queue() removes done jobs from queue and reorders the pending tasks
 * @args: structure of type thread_queue
 * @desc: This function takes queue of the thread which needs reordering.
 * It holds a lock during reordering as the contents are being changed, soin lock
 * is held for the life cycle of this function.
 * It copies the old order into a temporary queue, later it iterates throught
 * temporary queue and as it does that it checks the qSlotsStats which holds 
 * which job in the queue are done and needs to be seperated from not done jobs
 * @return: array structure of type queue_job
 */
struct queue_job** reorder_queue(struct thread_queue *queue)
{
    int qsize = queue->totalJobsInQueue;
    struct queue_job *tempArr[qsize];
    struct queue_job *doneJobs[qsize];
    int i,j,k, head = queue->headPointer;
    
    sem_wait(&queue->threadResource);
    DEBUG_MSG(__func__,"Reordering with qsize:", qsize, " head:", head);
    for (i = 0; i < qsize; i++){
        tempArr[i] = queue->queueHead[head];
        head = queue->totalJobsInQueue % head;
    }

    for(i = 0, j = 0, k = 0; i < qsize; i++){
        if(queue->qSlotDone[i]){
            doneJobs[j++]  = tempArr[i];
        }else{
            queue->queueHead[k++] = tempArr[i];
        }
        queue->qSlotDone[i] = 0;
    }
    queue->needsReorder = 0;
    queue->totalJobsInQueue = k;
    sem_post(&queue->threadResource);

    return doneJobs;
}

/* @funcion: cycle_jobs() function cycles the jobs in the given queue
 * It increments the head of queue so in the assosiated task threads
 * nest run it picks up the next job to run.
 * All jobs are timesliced so each get a slice of cpu depending on priority
 * variable assigned to each job. some jobs however are non preemptable which
 * are onshot job they are not cycled they are immidiatly popped out of queue
 * once done.
 * 
 * @return: boolean
 *  desc: is queue full or not?
 */

int cycle_jobs(struct thread_queue *queue)
{
    if(queue->headPointer > queue->tailPointer){
        reorder_queue(queue);
        queue->headPointer = 0;
    } else {
        queue->headPointer++;
    }

    if(queue->totalJobsInQueue < QUEUE_SIZE)
        return QUEUE_ISFREE;
    return QUEUE_FULL;
}
void *sched_task(void *ptr)
{
    int i, j;
    struct thread_queue *queue;
    struct queue_job **doneJobs;

    if(threadPoolHead == NULL){
        DEBUG_ERR(__func__, "thread head is uninited cant procced any further!");
        return;
    }

    while(!sched_should_stop)
    {
        //check for doneJobs and reorder queue is nessary
        for(i = 0, j = 0; i < MAX_THREAD; i++){
            queue = list[i];
            if(queue->needsReorder){
                doneJobs = reorder_queue(queue);
                while(doneJobs[j++]){
                    //dispose of jobs here
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
int init_sched(void){
    pthread_t sched_thread, *task_thread;
    struct thread_queue *queue;
    int i, ret, pid;

    for(i = 0; i < MAX_THREAD; i++)
    {
        task_thread = new pthread_t;
        queue = new thread_queue;
        if(queue == NULL){
            DEBUG_ERR(__func__,"queue alloc failed");
            return EXIT_FAILURE;
        }
        memset(&queue, 0, sizeof(thread_queue));
        sem_init(&queue->threadResource, 0 ,1);
        queue->queueHead[0] = NULL;
        list[i] = queue;
        
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