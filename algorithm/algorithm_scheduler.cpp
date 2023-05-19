#include <stdlib.h>
#include "algorithm_scheduler.hpp"
#include "../instance/instance.hpp"
#include "../data_processor/data_processor.hpp"
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"

InstanceList globalInstanceList;

int sched_algo(struct ThreadPool *thread, TableData *tData)
{
    ProcessStates* proc;
    short algoIndex;

    if(!tData){
        Log().error(__func__, "table data is uninitlised!");
        return EXIT_FAILURE;
    }

    algoIndex = globalInstanceList.getInstanceFromId(tData->instanceType).getLinkedAlgorithm();
    proc = avial_algo[algoIndex](tData);
    scheduleTask(thread, proc, tData, tData->priority);

    Log().info(__func__, "rows:", tData->metadata->rows, " Cols:", 
        tData->metadata->columns);
    Log().info(__func__, "Sched algorihm type:", algoIndex);

    return EXIT_SUCCESS;
}

void dealloc_table_dat(struct TableData *tData)
{
    //Don't care if it fails
     FileDataBaseAccess *fileDataBaseAccess = new FileDataBaseAccess(tData->tableID, RW_FILE);
    fileDataBaseAccess->dropFile();
    delete tData;
}