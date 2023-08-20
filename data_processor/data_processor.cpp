#include <string>
#include <algorithm>
#include <vector>
#include "../instance/instance_list.hpp"
#include "../include/packet.hpp"
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../include/task.hpp"
#include "../include/flag.h"
#include "../include/logger.hpp"
#include "../services/file_database_access.hpp"
#include "../sender_proc/sender.hpp"
#include "../scheduler/task_pool.hpp"
#include "data_processor.hpp"
#include "instance_data.hpp"
#include "clean_stage_tracker.hpp"

class DataProcessor {
    private:
        std::uint64_t curCol;
        std::string reFeilds;
        std::vector<std::string> colHeaders;
        std::string selectCmd;
        std::string deleteCmd;
        std::string errorString;
    public:
        InstanceData *instanceData;
        Flag initDone;
        Flag dataCleanPhase;
        std::uint64_t curRow = 1;
        TableData *tableData;
        FileDataBaseAccess *fileDataBaseAccess;
        
        DataProcessor(DataProcessContainer);
        ~DataProcessor();
        int initlize();
        int processSql(std::vector<std::string> feildList);
        std::string cleanData(std::string);
        //std::string buildUpdateCmd(std::string *);
        void buildSelectCmd();
        std::string validateFeild(std::string feild);
        std::string getErrorString() {return errorString;}
        int deleteDuplicateRecords();

};

DataProcessor::DataProcessor(DataProcessContainer container) : tableData(container.tData)
{
    //CSV Rows begins data after 0th row
    curRow = 1;
    Log().dataProcInfo(__func__, "tdata:", this->tableData->instanceType);
    initDone.initFlag(false);
    dataCleanPhase.initFlag(false);
}

DataProcessor::~DataProcessor()
{
    delete fileDataBaseAccess;
}

int DataProcessor::initlize()
{
    Log().dataProcInfo(__func__, "table data:", tableData->instanceType);
    Instance instance = globalInstanceList.getInstanceFromId(this->tableData->instanceType);
    if(instance.getId().empty()){
        Log().error(__func__, "invalid instance choosen: ", tableData->instanceType);
        return -1;
    }

    instanceData = new InstanceData(instance, tableData->metadata->columns, tableData->tableID);
    if(instanceData->initlizeData()){
        errorString = instanceData->getErrorString();
        return -1;
    }

    fileDataBaseAccess = new FileDataBaseAccess(tableData->tableID, RW_FILE);
    initDone.setFlag();
    colHeaders = fileDataBaseAccess->getColumnNamesList();
    Log().dataProcInfo(__func__, "Data processor setup and ready for validtaion/processing");
    return 0;
}

/* process_data_start(): This method is called by scheduler, It processes the data before algorithms work on it.
 * This method has two phases processSql phase where the sql is processed to look for invalid values in all columns, in 
 * second phase the data is cleaned and duplicate records are deleted from db.
*/
JobStatus process_data_start(void *data)
{
    DataProcessor *dataProc = (DataProcessor*)data;
    struct TableData *tData = dataProc->tableData;
    std::vector<std::string> feild;
    int rc;

    rc = dataProc->initlize();
    if(rc)
        return JOB_FAILED;

    return JOB_DONE;
}

JobStatus process_data_pause(void *data)
{
    DataProcessor *dataProc = (DataProcessor*)data;
    TableData *tData = dataProc->tableData;
    // This is just an update packet sent to server informing it the data is still being processed
    if(dataProc)
        senderSink.pushPacket("", tData->tableID, RESET_TIMER, tData->priority);

    return JOB_PENDING;
}

void process_data_finalize(void *data)
{
    DataProcessor *dataProc = (DataProcessor*)data;
    schedule_clean_phase(dataProc->tableData, dataProc->instanceData);

    //Deallocate memory and cleanup
    delete dataProc;
}

void process_data_failed(void *data)
{
    DataProcessor *dataProc = (DataProcessor*)data;
    TableData *tData = dataProc->tableData;
    
    Log().debug(__func__, "server will be notified of wrong data");
    senderSink.pushPacket(dataProc->getErrorString(), tData->tableID, DAT_ERR, tData->priority);
    // Cleanup before exit
    dealloc_table_dat(tData);
}

struct ProcessStates* data_proc = new ProcessStates {
    .name = "unsplit data processor",
    .type = DATAPROCESSOR_STAGE,
    .start_proc = process_data_start,
    .pause_proc = process_data_pause,
    .end_proc = process_data_finalize,
    .fail_proc = process_data_failed
};

int init_data_processor(DataProcessContainer container)
{
    DataProcessor *dpContainer = new DataProcessor(container);
    scheduleTask(data_proc, dpContainer, container.tData->priority);
    Log().dataProcInfo(__func__, "initilized data processor");

    return 0;
}