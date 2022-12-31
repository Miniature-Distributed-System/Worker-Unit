#include <stdlib.h>
#include "algo.hpp"
#include "../data_processing.hpp"
#include "../include/debug_rp.hpp"

int sched_algo(struct thread_pool *thread, struct table *tData)
{
    struct process* proc;
    short algoIndex;

    if(!tData){
        DEBUG_ERR(__func__, "table data is uninitlised!");
        return EXIT_FAILURE;
    }

    algoIndex = tData->algorithmType;
    proc = avial_algo[algoIndex](tData);
    sched_task(thread, proc, tData, tData->priority);

    DEBUG_MSG(__func__, "Sched algorihm type:",algoIndex);

    return EXIT_SUCCESS;
}