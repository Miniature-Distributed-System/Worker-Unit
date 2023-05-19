#include <boost/algorithm/string.hpp>
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
#include "../services/sqlite_database_access.hpp"
#include "../services/file_database_access.hpp"
#include "../algorithm/algorithm_scheduler.hpp"
#include "../sender_proc/sender.hpp"
#include "data_processor.hpp"
#include "instance_data.hpp"

class DataProcessor {
    private:
        std::uint64_t curCol;
        std::string reFeilds;
        std::vector<std::string> colHeaders;
        std::string selectCmd;
        std::string deleteCmd;
        std::string errorString;
        InstanceData *instanceData;
    public:
        Flag initDone;
        Flag dataCleanPhase;
        std::uint64_t curRow = 1;
        TableData *tableData;
        ThreadPool *thread;
        FileDataBaseAccess *fileDataBaseAccess;
        
        DataProcessor(ThreadPool*, DataProcessContainer);
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

DataProcessor::DataProcessor(struct ThreadPool* thread,
                DataProcessContainer container) : tableData(container.tData), thread(thread)
{
    //CSV Rows begins data after 0th row
    curRow = 1;
    Log().info(__func__, "tdata:", this->tableData->instanceType);
    initDone.initFlag(false);
    dataCleanPhase.initFlag(false);
}

DataProcessor::~DataProcessor()
{
    delete fileDataBaseAccess;
}

int DataProcessor::initlize()
{
    Log().info(__func__, "table data:", tableData->instanceType);
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
    Log().info(__func__, "Data processor setup and ready for validtaion/processing");
    return 0;
}

// validateFeild(): This is used by cleanData() to valdiate each field with the possible set of values.
std::string DataProcessor::validateFeild(std::string feild)
{
    std::string* temp;
    int i ,totalColumns;

    temp = instanceData->getPossibleFields(curCol);
    if(temp == NULL){
        Log().error(__func__, "Instance data for instance column index: ", curCol);
        return feild;
    }
    totalColumns = instanceData->getTotalRows();

    //iterate through possible values of current column
    for(i = 0; i < totalColumns; i++){
        if(boost::iequals(temp[i], feild)){
            return feild;
        }
    }

    //worst case scenario
    Log().info(__func__, "feild does not exist:", feild);
    return "NaN";
}

// cleanData(): This method is used by the processSql() to replace invalid data fields with NaN.
std::string DataProcessor::cleanData(std::string feild)
{
    //check if empty
    if(feild.empty())
        feild = "NaN";
    else {
        //check if valid feild present
        feild = validateFeild(feild);
    }
    
    return feild;
}

/* processSql(): This method cleans the attribute data in the sqlite database.
 * This method fetches the possible values from the algorithm and proceeds to check the sqlite database for inconsistent
 * data which it substitues with NaN which will be an invalid data.
*/
int DataProcessor::processSql(std::vector<std::string> feildList)
{
    int i, cols = tableData->metadata->columns, rc;
    std::vector<std::string> temp;
    std::string sqlUpdateCmd;

    curCol = 0;
    for(i = 0; i < cols; i++,curCol++){
        std::string str = cleanData(feildList[i]);
        temp.push_back(str);
    }
    if(temp != feildList){
        Log().info(__func__, "row:", curRow);
        fileDataBaseAccess->writeRowValueList(temp, curRow);
    }

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

    if(!dataProc->initDone.isFlagSet()){
        rc = dataProc->initlize();
        if(rc)
            return JOB_FAILED;
    }

    if(dataProc->dataCleanPhase.isFlagSet())
    {
        // Keep incrementing until we run out of records to delete
        rc = dataProc->fileDataBaseAccess->deleteDuplicateRecords(dataProc->curRow++);
        if(rc == -3){
            Log().info(__func__,"All duplicate data deleted");
            tData->metadata->currentColumn = 0;
            tData->metadata->rows = dataProc->fileDataBaseAccess->getTotalRows() - 1;
            return JOB_DONE;
        }
    } else {
        if(dataProc->curRow >= tData->metadata->rows){
            dataProc->dataCleanPhase.setFlag();
            dataProc->curRow = 0;
        } else {
            feild = dataProc->fileDataBaseAccess->getRowValueList(dataProc->curRow);
            if(feild.size() > 0){
                dataProc->processSql(feild);
                dataProc->curRow++;
        }
        }
    }

    return JOB_PENDING;
}

JobStatus process_data_finalize(void *data, JobStatus status)
{
    DataProcessor *dataProc = (DataProcessor*)data;
    struct TableData *tData = dataProc->tableData;
    std::string selectAll = "SELECT * FROM " + tData->tableID + ";";
    std::string getCleanedTable;

    if(status == JOB_FAILED){
        send_packet(dataProc->getErrorString(),tData->tableID, 
                        DAT_ERR, tData->priority);
        return JOB_FINISHED;
    }

    //Send the cleaned data back to server via fwd stack
    getCleanedTable = dataProc->fileDataBaseAccess->getBlob();
    Log().info(__func__, getCleanedTable);
    send_packet(getCleanedTable, tData->tableID, INTR_SEND, tData->priority);
    //Schedule the algorithm to process our cleaned data
    sched_algo(dataProc->thread, tData);

    //Deallocate memory and cleanup
    delete dataProc;

    return JOB_FINISHED;
}

struct ProcessStates* data_proc = new ProcessStates {
    .start_proc = process_data_start,
    .end_proc = process_data_finalize
};

int init_data_processor(struct ThreadPool* thread, DataProcessContainer container)
{
    DataProcessor *dpContainer = new DataProcessor(thread, container);
    scheduleTask(thread, data_proc, dpContainer, container.tData->priority);
    Log().info(__func__, "initilized data processor");

    return 0;
}