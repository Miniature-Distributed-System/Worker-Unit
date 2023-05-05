#include <stdlib.h>
#include <iostream>
#include "../include/debug_rp.hpp"
#include "../scheduler/sched.hpp"
#include "../include/process.hpp"
#include "../algorithm/algo.hpp"
#include "../data_processor/data_processor.hpp"
#include "../sql_access.hpp"
#include <unistd.h>

struct ThreadPool *thread;

//Dead functions for test compilation purposes
int send_packet(std::string data, std::string tableID, int statusCode)
{
    return 0;
}

struct table* tab_generator(int prio){
    struct table* tab;
    tab = new table;
    struct table_metadata *meta = new table_metadata;
    meta->cols = 7;
    meta->rows = 10;
    tab->algorithmType = 1;
    tab->metadata = meta;
    tab->priority = prio;
    tab->tableID = "mytable2";
    tab->dataLen = 10;

    return tab;
}

int main()
{
    int rc = 0, i,j, totalQJobs = 7;
    struct table *tab;
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "-----------Starting Sched Unit Test--------------" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    sleep(2);
    thread = init_thread_pool();
    if(!thread){
        DEBUG_ERR(__func__, "thread not inited");
        return 0;
    }
    init_db();
    for(i = 0; i < totalQJobs; i++){
        tab = tab_generator(3);
        sched_algo(thread, tab);
    }
    rc = thread->threadPoolCount;
    DEBUG_MSG(__func__,"inserted into pool");
    DEBUG_MSG(__func__, "");
    init_sched(thread);
    DEBUG_MSG(__func__,"going to sleep");
    DEBUG_MSG(__func__, "");
    sleep(2);
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "-----------Adding Additional Jobs-----------------" << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    for(i = 0; i < totalQJobs; i++){
        tab = tab_generator(2);
        sched_algo(thread, tab);
    }
    DEBUG_MSG(__func__, "");
    sleep(2);
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << "-----------Ending Unit Test-----------------" << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    return 0;
}