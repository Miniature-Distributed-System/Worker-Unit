#include <boost/algorithm/string.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include "algorithm/algo.hpp"
#include "algorithm/instance.hpp"
#include "instance/instance_list.hpp"
#include "data_processor.hpp"
#include "sql_access.hpp"
#include "include/packet.hpp"
#include "include/process.hpp"
#include "include/debug_rp.hpp"
#include "include/task.hpp"
#include "include/flag.h"
#include "sender_proc/sender.hpp"

class InstanceData {
        Instance instance;
        std::vector<std::string*> instanceDataMatrix;
        std::string userTableName;
        int userTableTotalColumns;
        std::string errorString;
        //DatabaseAccess *dataBaseAccess;
    public:
        InstanceData(Instance, int, std::string);
        ~InstanceData();
        int initlizeData();
        std::string* validateColumns();
        std::string* getPossibleFields(int column);
        std::uint64_t getTotalRows() { return instance.getTotalRows(); }
        std::string getErrorString() { return errorString;}
};

InstanceData::InstanceData(Instance instance, int cols, std::string userTableId)
{
    this->instance = instance;
    userTableTotalColumns = cols;
    userTableName = userTableId;
}

InstanceData::~InstanceData()
{
    //delete dataBaseAccess;
}

std::string* InstanceData::validateColumns()
{
    int k = 0, cols = instance.getTotalColumns();

    if(cols != userTableTotalColumns){
        errorString = "columns dont match user: " + userTableTotalColumns;
        errorString += " instance: " + cols + 0;
        DEBUG_ERR(__func__, errorString);
        return NULL;
    }

    std::string *result = new std::string[cols];
    std::string *instanceColumnNames = dataBaseAccess->getColumnNames(instance.getId(), cols);
    std::string *userTableColumnNames = dataBaseAccess->getColumnNames(userTableName, userTableTotalColumns);

    if(instanceColumnNames == NULL || userTableColumnNames == NULL){
        errorString = "column data retrevial failed";
        DEBUG_ERR(__func__,errorString);
        return NULL;
    }

    //Match all columns else its an error
    for(int i = 0; i < cols; i++){
        for(int j = 0; j < cols; j++){
            if(!userTableColumnNames[i].compare(instanceColumnNames[j])){
                result[k++] = instanceColumnNames[j];
                break;
            }
        }
    }

    if(k != cols){
        errorString = "Instance table and User table columns did not match";
        DEBUG_ERR(__func__, errorString);
        return NULL;
    }

    DEBUG_MSG(__func__, "Columns validation successfull!");
    return result;
}

int InstanceData::initlizeData()
{
    std::string *str, *columnNames;
    // dataBaseAccess = new DatabaseAccess();
    // dataBaseAccess->initDatabase();

    columnNames = validateColumns();
    if(columnNames == NULL)
        return -1;
    for(int i = 0; i < instance.getTotalColumns(); i++){
        //DEBUG_MSG(__func__, "TableId:", instance.getId(), " ColName:", columnNames[i]);
        //Any failures here would be unfortunate and not correctable
        str = dataBaseAccess->getColumnValues(instance.getId(), columnNames[i], instance.getTotalRows());
        instanceDataMatrix.push_back(str);
    }

    DEBUG_MSG(__func__, "data initilize success!");
    return 0;
}

std::string* InstanceData::getPossibleFields(int column)
{
    return instanceDataMatrix.at(column);
}

class dataProcessor{
    private:
        std::uint64_t curCol;
        std::string reFeilds;
        std::string *colHeaders;
        std::string selectCmd;
        std::string deleteCmd;
        std::string errorString;
        InstanceData *instanceData;
    public:
        Flag initDone;
        Flag dataCleanPhase;
        std::uint64_t curRow = 1;
        struct table *tData;
        struct thread_pool *thread;
        //DatabaseAccess *dataBaseAccess;
        
        dataProcessor(struct thread_pool* ,
                        struct DataProcessContainer *);
        ~dataProcessor();
        int initlize();
        int processSql(std::string *);
        std::string cleanData(std::string);
        std::string buildUpdateCmd(std::string *);
        void buildSelectCmd();
        std::string validateFeild(std::string feild);
        std::string getErrorString() {return errorString;}
        int deleteDuplicateRecords();

};

dataProcessor::dataProcessor(struct thread_pool* thread,
                struct DataProcessContainer *container)
{
    this->tData = container->tData;
    this->thread = thread;
    //Sql IDs dont start with 0
    this->curRow = 1;

    colHeaders = container->colHeaders;
    initDone.initFlag(false);
    dataCleanPhase.initFlag(false);
    deleteCmd = "DELETE FROM " + tData->tableID + " WHERE ID=";
    //dealloc container bundle
    //dataBaseAccess = new DatabaseAccess();
}

dataProcessor::~dataProcessor()
{
    //delete dataBaseAccess;
}

int dataProcessor::initlize()
{
    Instance instance = globalInstanceList.getInstanceFromId(tData->instanceType);
    if(instance.getId().empty()){
        errorString = "Invalid instance choosen index: ",tData->instanceType;
        DEBUG_ERR(__func__, errorString);
        return -1;
    }

    instanceData = new InstanceData(instance, tData->metadata->cols, tData->tableID);
    if(instanceData->initlizeData()){
        errorString = instanceData->getErrorString();
        return -1;
    }
    initDone.setFlag();
    //dataBaseAccess->initDatabase();

    DEBUG_MSG(__func__, "Data processor setup and ready for validtaion/processing");
    return 0;
}

// validateFeild(): This is used by cleanData() to valdiate each field with the possible set of values.
std::string dataProcessor::validateFeild(std::string feild)
{
    std::string* temp;
    int i ,totalColumns;
    
    // Base* base = makers[tData->instanceType]();
    // aviCols = base->getColCount();
    // temp = base->getPossibleFeilds(curCol);
    // delete base;
    temp = instanceData->getPossibleFields(curCol);
    if(temp == NULL){
        DEBUG_ERR(__func__, "Instance data for instance column index: ", curCol);
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
    DEBUG_MSG(__func__, "feild does not exist:", feild);
    return "NaN";
}

// cleanData(): This method is used by the processSql() to replace invalid data fields with NaN.
std::string dataProcessor::cleanData(std::string feild)
{
    //check if empty
    if(feild.empty())
        feild = "NaN";
    else {
        //transform to lower case
        //check if valid feild present
        feild = validateFeild(feild);
    }
    
    return feild;
}

// buildUpdateCmd(): This method is used by the below method to build sql query to update selecte row.
std::string dataProcessor::buildUpdateCmd(std::string *newFeilds)
{
    std::string updateCmd = "UPDATE " + tData->tableID + " SET ";
    int cols = tData->metadata->cols;

    for(int i = 0; i < cols; i++){
        updateCmd += colHeaders[i] + "='" + newFeilds[i] + "'";
        if(i < cols - 1)
            updateCmd += ",";
    }
    updateCmd += " WHERE ID=" + std::to_string(curRow + 0) + ";";
    //DEBUG_MSG(__func__, "build update command for row ID:", curRow + 0);
    return updateCmd;
}

/* processSql(): This method cleans the attribute data in the sqlite database.
 * This method fetches the possible values from the algorithm and proceeds to check the sqlite database for inconsistent
 * data which it substitues with NaN which will be an invalid data.
*/
int dataProcessor::processSql(std::string *feild)
{
    int i, cols = tData->metadata->cols;
    std::string* temp = new std::string[cols];
    std::string sqlUpdateCmd;
    bool modified = false;

    curCol = 0;
    for(i = 0; i < cols; i++,curCol++){
        temp[i] = cleanData(feild[i]);
        if(feild[i] != temp[i])
            modified = true;
    }
    if(modified){
        sqlUpdateCmd = buildUpdateCmd(temp);
        dataBaseAccess->writeValue(sqlUpdateCmd.c_str());
        //DEBUG_MSG(__func__, "record ID:",curRow + 0," cleaned");
    }

    return 0;
}

// buildSelectCmd(): This method builds the sql query for selecting of rows for deleting purposes
void dataProcessor::buildSelectCmd()
{
    selectCmd = "SELECT ID,";
    int i, cols = tData->metadata->cols;

    for(i = 0; i < cols; i++){
        selectCmd += colHeaders[i] + ',';
    }
    selectCmd += "count(*) FROM " + tData->tableID + " GROUP BY ";
    for(i = 0; i < cols; i++){
        selectCmd += colHeaders[i];
        if(i < cols - 1)
            selectCmd += ',';
    }
    selectCmd += " having count(*) > 1;";
}

// deleteDuplicateRecords(): This method cycles through the db and finds duplicate records and deletes them.
int dataProcessor::deleteDuplicateRecords()
{
    std::string *rowID;
    std::string temp;

    if(selectCmd.empty())
        buildSelectCmd();
    rowID = dataBaseAccess->readValue(selectCmd.c_str(), 0);
    if(!rowID){
        DEBUG_ERR(__func__, "select command read error");
        return 0;
    }
    if(!rowID->empty())
    {
        //DEBUG_MSG(__func__, "cleaning up duplicate record ID:", *rowID);
        temp = deleteCmd + *rowID + ";";
        dataBaseAccess->writeValue(temp.c_str());
        return 0;
    }
    else
        return 1; 
}

/* process_data_start(): This method is called by scheduler, It processes the data before algorithms work on it.
 * This method has two phases processSql phase where the sql is processed to look for invalid values in all columns, in 
 * second phase the data is cleaned and duplicate records are deleted from db.
*/
JobStatus process_data_start(void *data)
{
    dataProcessor *dataProc = (dataProcessor*)data;
    struct table *tData = dataProc->tData;
    std::string *feild;
    int rc;

    if(!dataProc->initDone.isFlagSet()){
        rc = dataProc->initlize();
        if(rc)
            return JOB_FAILED;
    }

    if(dataProc->dataCleanPhase.isFlagSet())
    {
        rc = dataProc->deleteDuplicateRecords();
        if(rc){
            DEBUG_MSG(__func__,"All duplicate data deleted");
            tData->metadata->curCol = 0;
            return JOB_DONE;
        }
    } else {
        if(dataProc->curRow >= tData->metadata->rows)
            dataProc->dataCleanPhase.setFlag();
        feild = dataBaseAccess->getRowValues(tData, dataProc->curRow);
        //feild = get_row_values(tData, dataProc->curRow);
        if(feild){
            dataProc->processSql(feild);
            dataProc->curRow++;
        }
    }

    return JOB_PENDING;
}

JobStatus process_data_finalize(void *data, JobStatus status)
{
    dataProcessor *dataProc = (dataProcessor*)data;
    struct table *tData = dataProc->tData;
    std::string selectAll = "SELECT * FROM " + tData->tableID + ";";
    std::string *getCleanedTable;

    if(status == JOB_FAILED){
        send_packet(dataProc->getErrorString(),tData->tableID, 
                        DAT_ERR, tData->priority);
        return JOB_FINISHED;
    }

    //Send the cleaned data back to server via fwd stack
    // DatabaseAccess *dataBaseAccess = new DatabaseAccess();
    // dataBaseAccess->initDatabase();
    getCleanedTable = dataBaseAccess->readValue(selectAll.c_str(), -1);
    std::replace(getCleanedTable->begin(), getCleanedTable->end(), ';', '\n');
    //delete dataBaseAccess;
    //getCleanedTable = sql_read(selectAll.c_str(), -1);
    send_packet(*getCleanedTable, tData->tableID, INTR_SEND, tData->priority);

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

int init_data_processor(struct thread_pool* thread,
                struct DataProcessContainer* container)
{
    dataProcessor *dpContainer = new dataProcessor(thread, container);
    if(!thread) DEBUG_ERR(__func__, "error1");
    if(!data_proc) DEBUG_ERR(__func__, "error2");
    if(!dpContainer) DEBUG_ERR(__func__, "error3");
    if(!container) DEBUG_ERR(__func__, "error4");
    if(!container->tData) DEBUG_ERR(__func__, "error5");
    if(!container->tData->priority) DEBUG_ERR(__func__, "error6");
    scheduleTask(thread, data_proc, dpContainer, container->tData->priority);
    delete container;
    DEBUG_MSG(__func__, "initilized data processor");

    return 0;
}