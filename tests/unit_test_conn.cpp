#include <unistd.h>
#include "../socket/socket.hpp"
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../scheduler/thread_pool.hpp"
#include "../scheduler/sched.hpp"
#include "../sql_access.hpp"

struct thread_pool *thread;
struct datContainer{
    struct process *newProc;
    void *args;
};

void* proc(void *data)
{
    struct datContainer* newDat = (struct datContainer*)data;
    newDat->newProc->start_proc(newDat->args);
    newDat->newProc->end_proc(newDat->args);
}

int sched_task(struct thread_pool *threadPoolHead, struct process *newProc, 
                void *args, int prior)
{
    pthread_t pthread;
    struct datContainer* newDat = new datContainer;
    newDat->newProc = newProc;
    newDat->args = args;
    pthread_create(&pthread,NULL, proc, newDat);
    return 0;
}
int sched_algo(struct thread_pool* threadPool, struct table *tData)
{
    DEBUG_MSG(__func__, "algorithm scheduled");
    return 0;
}

int init_data_processor(struct thread_pool* thread,
                struct data_proc_container* container){
    return 0;
}

int main()
{
    struct socket *soc;
    std::string args[2];
    args[0] = "0.0.0.0";
    args[1] = "8080";
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "-----------Starting Socket Unit Test-------------" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    soc = init_socket(thread, args);
    init_db();
    sleep(50);
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << "-----------Ending Unit Test-----------------" << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    return 0;
}