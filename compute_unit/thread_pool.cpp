#include "thread_pool.hpp"
#include "receiver_proc/debug_rp.hpp"

sem_t threadPool_mutex;
struct thread_pool *threadPoolHead;
unsigned int threadPoolCount;

int init_thread_pool()
{
    threadPoolHead = new thread_pool;
    sem_init(&threadPool_mutex, 0, 1);
    threadPoolCount = 0;
    threadPoolHead->pData = nullptr;
    threadPoolHead->next = nullptr;
    DEBUG_MSG(__func__, "Inited thread pool");

    return 0;
}

void exit_thread_pool(){
    struct thread_pool *temp, *temp_head;

    temp = temp_head = threadPoolHead;
    while(temp_head != nullptr){
        temp = temp_head;
        temp_head = temp->next;
        delete temp;
    }
    sem_destroy(&threadPool_mutex);
    DEBUG_MSG(__func__, "de-allocated thread pool data");
}

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
int insert_node(struct process_table* procTable)
{
    struct thread_pool *node, *curHead, *shiftNode;
    sem_wait(&threadPool_mutex);
    if(threadPoolCount >= MAX_POOL_SIZE){
        DEBUG_ERR(__func__,"Max pool limit reached!");
        sem_post(&threadPool_mutex);
        return EXIT_FAILURE;
    }
    
    node = new thread_pool;
    node->pData = procTable;
    curHead = threadPoolHead->next;
    
    //Single node in list
    if(curHead == nullptr){
        DEBUG_MSG(__func__, "first node of thread pool created");
        threadPoolHead->next = node;
        node->next = nullptr;
    }
    else{
        while(curHead != nullptr){
            if(procTable->priority < curHead->pData->priority){
                if(curHead->pData->starveCounter >=
                        get_starve_limit(curHead->pData->priority))
                            continue;
                else{
                    shiftNode = curHead->next;
                    curHead->next = node;
                    node->next = shiftNode;
                    while(node->next != nullptr){
                        node->pData->starveCounter++;
                        node = node->next;
                    }
                    break;
                }
                curHead = curHead->next;
            }
        }
    }
    threadPoolCount++;
    DEBUG_MSG(__func__, "process inserted into thread pool");
    sem_post(&threadPool_mutex);
    
    return 0;
}

void delete_node()
{
    sem_wait(&threadPool_mutex);
    //This condition should always be false if its true we fucked up somewhere
    if(threadPoolCount < 1){
        DEBUG_ERR(__func__,"Thread pool is already empty!");
        threadPoolCount = 0;
        sem_post(&threadPool_mutex);
        return;
    }
    struct thread_pool *curHead = threadPoolHead->next, *nextHead;
    
    nextHead = curHead->next;
    delete curHead;
    threadPoolHead->next = nextHead;
    threadPoolCount--;
    sem_post(&threadPool_mutex);
}

/* This function initilises the process table for the submitted
 * process along with null checks. Once inited the process table
 * is pushed into the thread_pool.
 * In case of failure it returns EXIT_FAILURE macro else 0.
 */
int sched_task(struct process *newProc, void *args, int prior)
{
    struct process_table *newProcTab;
    int rc = 0;
    
    if(newProc == nullptr || args == nullptr){
        return EXIT_FAILURE;
    }

    newProcTab = new process_table;
    newProcTab->proc = newProc;
    newProcTab->args = args;
    newProcTab->priority = prior;
    newProcTab->starveCounter = 0;
    DEBUG_MSG(__func__,"insert node");
    rc = insert_node(newProcTab);

    return rc;
}

