#include "../include/process.hpp"
#include "../include/logger.hpp"
#include "../services/file_database_access.hpp"

class DataCleaner{
        int iterator;
        FileDataBaseAccess *fileDataBaseAccess;
        TableData* tableData;
    public:
        DataCleaner(TableData* tableData);
        ~DataCleaner();
        int clean();
};

DataCleaner::DataCleaner(TableData* tableData)
{
    fileDataBaseAccess = new FileDataBaseAccess(tableData->tableID, RW_FILE);
    iterator = 0;
    Log().info(__func__,"initlized data cleaner");
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

JobStatus process_data_start(void *data)
{
    DataCleaner *dataCleaner = (DataCleaner*)data;
    if(dataCleaner->clean())
        return JOB_DONE;
    return JOB_PENDING;
}

JobStatus process_data_pause(void *data)
{
   return JOB_PENDING;
}

void process_data_finalize(void *data)
{
    DataCleaner *dataCleaner = (DataCleaner*)data;

    delete dataCleaner;
    Log().info(__func__,"finished data cleaner");
}

void process_data_failed(void *data)
{
   DataCleaner *dataCleaner = (DataCleaner*)data;
   delete dataCleaner;
}

struct ProcessStates* data_proc = new ProcessStates {
    .name = "split data cleaner",
    .type = DATAPROCESSOR_STAGE,
    .start_proc = process_data_start,
    .pause_proc = process_data_pause,
    .end_proc = process_data_finalize,
    .fail_proc = process_data_failed
};

void init_data_cleaner(TableData* tableData)
{
    DataCleaner *dataCleaner = new DataCleaner(tableData);

}
