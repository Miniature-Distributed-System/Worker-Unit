#include "../include/process.hpp"
#include "../include/logger.hpp"
#include "../services/file_database_access.hpp"
#include "../sender_proc/sender.hpp"
#include "../algorithm/algorithm_scheduler.hpp"
#include "instance_data.hpp"
#include "data_tracker.hpp"

class DataCleaner{
        int iterator;
        FileDataBaseAccess *fileDataBaseAccess;
    public:
        TableData* tableData;
        InstanceData *instance;
        DataCleaner(TableData* tableData, InstanceData *instance);
        ~DataCleaner();
        int clean();
        std::string getCleanedData() { return fileDataBaseAccess->getBlob(); }
};

DataCleaner::DataCleaner(TableData* tableData, InstanceData *instance) : tableData(tableData), instance(instance)
{
    fileDataBaseAccess = new FileDataBaseAccess(tableData->tableID, RW_FILE);
    iterator = 0;
    Log().dataProcInfo(__func__,"initlized data cleaner");
}

DataCleaner::~DataCleaner()
{
    delete fileDataBaseAccess;
}

int DataCleaner::clean()
{
    if(fileDataBaseAccess->deleteDuplicateRecords(iterator++) == DUPLICATE_CLEANUP_DONE)
        return 1;
    return 0;
}

JobStatus clean_data_start(void *data)
{
    DataCleaner *dataCleaner = (DataCleaner*)data;
    if(dataCleaner->clean())
        return JOB_DONE;
    return JOB_PENDING;
}

JobStatus clean_data_pause(void *data)
{
   return JOB_PENDING;
}

void clean_data_finalize(void *data)
{
    DataCleaner *dataCleaner = (DataCleaner*)data;
    std::string cleanedData = dataCleaner->getCleanedData();
    
    schedule_validate_phase(dataCleaner->tableData, dataCleaner->instance);
    delete dataCleaner;
    Log().dataProcInfo(__func__,"finished data cleaner");
}

void clean_data_failed(void *data)
{
   DataCleaner *dataCleaner = (DataCleaner*)data;
   delete dataCleaner;
}

struct ProcessStates* clean_proc = new ProcessStates {
    .name = "serial data cleaner",
    .type = DATAPROCESSOR_STAGE,
    .start_proc = clean_data_start,
    .pause_proc = clean_data_pause,
    .end_proc = clean_data_finalize,
    .fail_proc = clean_data_failed
};

void init_data_cleaner(TableData* tableData, InstanceData *instance)
{
    DataCleaner *dataCleaner = new DataCleaner(tableData, instance);
    scheduleTask(clean_proc, dataCleaner, tableData->priority);
}
