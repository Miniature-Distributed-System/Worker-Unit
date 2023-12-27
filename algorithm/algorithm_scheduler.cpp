#include <stdlib.h>
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../configs.hpp"
#include "../instance/instance.hpp"
#include "../data_processor/data_processor.hpp"
#include "../services/file_database_access.hpp"
#include "../services/global_objects_manager.hpp"
#include "algorithm_scheduler.hpp"

InstanceList globalInstanceList;

/* sched_algo(): schedules the next stage in data processing in pipeline.
* This method takes tableData and splits the data into number of threads available. These split data are fed to the 
* algorithm that is specified for processing of given data. These split stages are indipenedent of each other. The
* algorithm is detected from passed TableData information. 
*/
int sched_algo(TableData *tData)
{
    ProcessStates* proc;
    short algoIndex;

    if(!tData){
        Log().error(__func__, "table data is uninitlised!");
        return EXIT_FAILURE;
    }

    algoIndex = globalInstanceList.getInstanceFromId(tData->instanceType).getLinkedAlgorithm();
    
    int totalThreads = globalObjectsManager.get<Configs>().getTotalThreadCount();
    int endRowNumber = tData->metadata->rows / totalThreads;
    int startRowNumber = 1;
    AlgorithmPackage *algoPackage = new AlgorithmPackage;
    algoPackage->tableData = tData;
    algorithmResultMap.insert(std::make_pair(tData->tableID, algoPackage));

    // Divide dataset based on total threads available
    for(int i = 1; i < totalThreads; i++){
        AlgorithmExportPackage package = avial_algo[algoIndex](tData->tableID, tData->instanceType, startRowNumber, endRowNumber * i, tData->metadata->columns);
        scheduleTask(package.proc, package.args, tData->priority);
        startRowNumber = endRowNumber * i + 1;
        Log().info(__func__, "end loop:", startRowNumber);
    }
    // Also account for odd number of rows
    AlgorithmExportPackage package = avial_algo[algoIndex](tData->tableID, tData->instanceType, startRowNumber, tData->metadata->rows, tData->metadata->columns);
    scheduleTask(package.proc, package.args, tData->priority);
    Log().info(__func__, "end:", startRowNumber);

    Log().info(__func__, "rows:", tData->metadata->rows, " Cols:", 
        tData->metadata->columns);
    Log().info(__func__, "Sched algorihm type:", algoIndex);

    return EXIT_SUCCESS;
}

/* update_algo_result(): Keeps track of all split stage and schedules aggrigate phase when all splits are done.
* This method updates the counter which keeps track of all split stages for a given data and once all split stages
* have completed/updated the result, this method schedules the final/aggrigate stage. It also deallocates all
* resources that were used during these split scheduling and tracking processes.
*/
int update_algo_result(std::string tableId, void *algorithmExportResult)
{
    if(tableId.empty()){
        Log().debug(__func__, "tableId is empty");
        return EXIT_FAILURE;
    }

    auto iterator = algorithmResultMap.find(tableId);
    if(iterator != algorithmResultMap.end()){
        iterator->second->resultVectors.push_back(algorithmExportResult);
        Log().info(__func__, "size:", iterator->second->resultVectors.size());
        if(iterator->second->resultVectors.size() == globalObjectsManager.get<Configs>().getTotalThreadCount())
        {
            TableData *tableData = iterator->second->tableData;
            int algoIndex = globalInstanceList.getInstanceFromId(tableData->instanceType)
                .getLinkedAlgorithm();
            AlgorithmExportPackage finalizeExportPackage = avialable_finalize_algo[algoIndex]
                (iterator->second->resultVectors, tableData->tableID, tableData->priority);
            scheduleTask(finalizeExportPackage.proc, finalizeExportPackage.args, NON_PREEMTABLE);
            Log().info(__func__, "tableId:", tableData->tableID, 
                " finalize process scheduled with priority:", tableData->priority);
            // Cleanup everything
            globalObjectsManager.get<DataBaseInstanceList>().dereferenceInstance(tableData->tableID);
            dealloc_table_dat(tableData);
            delete iterator->second;
            algorithmResultMap.erase(iterator);
        }
        return 0;
    }

    Log().debug(__func__, "could not find mapped entry for tableId:", tableId);
    return EXIT_FAILURE;
}

void dealloc_table_dat(struct TableData *tData)
{
    //Don't care if it fails
    FileDataBaseAccess *fileDataBaseAccess = new FileDataBaseAccess(tData->tableID, RW_FILE);
    fileDataBaseAccess->dropFile();
    delete tData;
}
