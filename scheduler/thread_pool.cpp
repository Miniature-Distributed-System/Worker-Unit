#include <unistd.h>
#include "thread_pool.hpp"
#include "sched.hpp"
#include "../include/packet.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../sender_proc/sender.hpp"
#include "../services/stats_engine.hpp"

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

/* This function inserts node into thread pool.
 * It checks for pool current size if full returns failure else it inserts the 
 * procTable into the thread pool. Depending on proccess priority the procTable 
 * is inserted into the thread pool, if process is having priority greater than 
 * lowest priority the process is compared with rest of the processes in table 
 * and inserted in the end of its peers with same priority and all lower 
 * priority process have their starve counter incremented.
 * If next process is found be starved longer than its limit the higher prioriy
 * process is pushed back until we hit a non starved process, and then inserted.
 */
int insert_node(struct ThreadPool* threadPoolHead, 
                TaskData* procTable)
{
    struct ThreadPoolNode *node, *curHead, *shiftNode;
    
    sem_wait(&threadPoolHead->threadPool_mutex);
    if(threadPoolHead->threadPoolCount >= MAX_POOL_SIZE){
        Log().info(__func__,"Max pool limit reached!");
        sem_post(&threadPoolHead->threadPool_mutex);
        return EXIT_FAILURE;
    }

    node = new ThreadPoolNode;
    node->pData = procTable;
    node->next = NULL;
    curHead = threadPoolHead->headNode;
    
    //Single node in list
    if(curHead == NULL){
        Log().info(__func__, "first node of thread pool created");
        threadPoolHead->headNode = node;
        node->next = NULL;
    }
    else{
        
        while(curHead){
            if(procTable->priority < curHead->pData->priority)
            {
                if(curHead->pData->starveCounter >=
                        get_starve_limit(curHead->pData->priority)){
                            Log().info(__func__, "node is starved skipping...");
                            continue;
                        }
                else{
                    shiftNode = curHead->next;
                    curHead->next = node;
                    node->next = shiftNode;
                    while(node->next != NULL){
                        node->pData->starveCounter++;
                        node = node->next;
                    }
                    break;
                }
            }

            if(curHead->next == NULL){
                Log().info(__func__, "add to bottom of list");
                curHead->next = node;
                break;
            }
            curHead = curHead->next;
        }
    }

    threadPoolHead->threadPoolCount++;
    Log().info(__func__, "pushed into pool, threadPoolCount:",
                threadPoolHead->threadPoolCount + 0);
    sem_post(&threadPoolHead->threadPool_mutex);
    
    return 0;
}

void delete_node(struct ThreadPool* threadPoolHead)
{
    struct ThreadPoolNode *curHead, *nextHead;

    sem_wait(&threadPoolHead->threadPool_mutex);
    //This condition should always be false if its true we fucked up somewhere
    if(threadPoolHead->threadPoolCount < 1)
    {
        Log().info(__func__,"Thread pool is already empty!");
        threadPoolHead->threadPoolCount = 0;
        sem_post(&threadPoolHead->threadPool_mutex);
        return;
    }
    
    curHead = threadPoolHead->headNode;
    nextHead = curHead->next;
    delete curHead;
    threadPoolHead->headNode = nextHead;
    threadPoolHead->threadPoolCount--;

    Log().info(__func__,"popped node from thread pool cur cnt:", 
            threadPoolHead->threadPoolCount + 0);
    sem_post(&threadPoolHead->threadPool_mutex);
}

/* This function initilises the process table for the submitted
 * process along with null checks. Once inited the process table
 * is pushed into the thread_pool.
 * In case of failure it returns EXIT_FAILURE macro else 0.
 */
int scheduleTask(struct ThreadPool *threadPoolHead, ProcessStates *newProc, 
                void *args, TaskPriority prior)
{
    TaskData *newProcTab;
    int rc = 0;
    
    if(newProc == NULL || args == NULL){
        return EXIT_FAILURE;
    }
    if(newProc->start_proc == NULL || newProc->end_proc == NULL)
        return EXIT_FAILURE;

    newProcTab = new TaskData;
    newProcTab->proc = newProc;
    newProcTab->args = args;
    newProcTab->priority = prior;
    newProcTab->starveCounter = 0;
    
    //DEBUG_MSG(__func__,"insert node");
    rc = insert_node(threadPoolHead, newProcTab);
    statsEngine.updateThreadStats(newProcTab->priority, TASK_IN);
    statsEngine.updateTimerBefore();
    pthread_cond_signal(&cond);
    
    return rc;
}

TaskData thread_pool_pop(struct ThreadPool* threadPoolHead)
{   
    TaskData temp; 
    struct ThreadPoolNode *tempHead;
    
    if(!threadPoolHead->headNode){
        Log().debug(__func__, "list is empty");
        return temp;
    }
    
    tempHead = threadPoolHead->headNode;
    temp.proc = tempHead->pData->proc;
    temp.args = tempHead->pData->args;
    threadPoolHead->headNode = tempHead->next;
    threadPoolHead->threadPoolCount--;
    delete tempHead->pData;
    delete tempHead;
    Log().info(__func__, "popping job from pool pcnt:", 
                threadPoolHead->threadPoolCount + 0);

    statsEngine.updateThreadStats(temp.priority, TASK_OUT);
    statsEngine.updateTimerAfter();
    
    return temp;
}

struct ThreadPool* init_thread_pool()
{
    struct ThreadPool* threadPoolHead = new ThreadPool;

    if(!threadPoolHead){
        Log().error(__func__,"Failed to alloc thread pool memory");
        return NULL;
    }

    sem_init(&threadPoolHead->threadPool_mutex, 0, 1);
    threadPoolHead->threadPoolCount = 0;
    threadPoolHead->headNode = NULL;

    return threadPoolHead;
}

void exit_thread_pool(struct ThreadPool* threadPoolHead){
    struct ThreadPoolNode *temp, *temp_head;

    /* send the seize packet which will let server know that node is about to 
       shutdown and no more csv data should be sent to node for processing. */
    senderSink.pushPacket("", "", SEIZE, HIGH_PRIORITY);
    //wait for task queue to empty itself before teardown
    while(threadPoolHead->threadPoolCount){
        sleep(2);
    }

    temp = temp_head = threadPoolHead->headNode;
    while(temp_head != NULL){
        temp = temp_head;
        temp_head = temp->next;
        delete temp;
    }
    sem_destroy(&threadPoolHead->threadPool_mutex);
    Log().info(__func__, "de-allocated thread pool data");
}

