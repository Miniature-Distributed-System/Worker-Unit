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