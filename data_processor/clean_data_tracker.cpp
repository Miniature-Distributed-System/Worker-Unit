#include "../include/logger.hpp"
#include "../sender_proc/sender.hpp"
#include "../algorithm/algorithm_scheduler.hpp"
#include "../services/file_database_access.hpp"
#include "../configs.hpp"
#include "clean_data.hpp"
#include "instance.hpp"
#include "clean_data_tracker.hpp"

void schedule_clean_phase(TableData *tableData, Instance *instance)
{
    int totalThreads = globalConfigs.getTotalThreadCount();
    int startIndex = 0, endIndex, multiplier = tableData->metadata->rows / totalThreads;
    endIndex = multiplier;
    
    for(int i = 0; i < totalThreads; i++){
        init_clean_data_phase(startIndex, endIndex, tableData, instance);
        startIndex = endIndex + 1;
        endIndex += multiplier * i;
        if(endIndex + 1 <= tableData->metadata->rows)
            endIndex++;
    }
    Log().info(__func__, "All clean stages scheduled");
    cleanStageMap[tableData] = 0;
}

int update_clean_stages(TableData *tableData)
{
    if(tableData == nullptr)
    {
        Log().error(__func__, "table Name is empty");
        return -1;
    }

    auto iterator = cleanStageMap.find(tableData);
    if(iterator != cleanStageMap.end()){
        iterator->second++;
        if(iterator->second >= globalConfigs.getTotalThreadCount()){
            std::string getCleanedTable;
            FileDataBaseAccess fileDataBaseAccess(tableData->tableID, READ_FILE);
            //Send the cleaned data back to server via fwd stack
            getCleanedTable = fileDataBaseAccess.getBlob();
            //Log().dataProcInfo(__func__, getCleanedTable);
            senderSink.pushPacket(getCleanedTable, tableData->tableID, INTR_SEND, tableData->priority);
            //Schedule the algorithm to process our cleaned data
            sched_algo(tableData);
        }
    } else {
        Log().error(__func__, "table Name not found!");
        return -1;
    }

    return 0;
}