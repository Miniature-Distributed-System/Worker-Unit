#include <string>
#include <algorithm>
#include "algorithm/algo.hpp"
#include "data_processor.hpp"
#include "sql_access.hpp"
#include "include/packet.hpp"
#include "include/process.hpp"
#include "include/debug_rp.hpp"
#include "include/task.hpp"
#include "sender_proc/sender.hpp"

class dataProcessor{
    private:
        std::uint64_t curCol;
        std::string reFeilds;
        std::string *colHeaders;
        std::string selectCmd;
        std::string deleteCmd;
    public:
        bool dataCleanPhase = false;
        std::uint64_t curRow = 1;
        struct table *tData;
        struct thread_pool *thread;

        dataProcessor(struct thread_pool* ,
                        struct data_proc_container *);
        int processSql(std::string *);
        std::string cleanData(std::string);
        std::string buildUpdateCmd(std::string *);
        void buildSelectCmd();
        std::string validateFeild(std::string feild);
        int deleteDuplicateRecords();

};

dataProcessor::dataProcessor(struct thread_pool* thread,
                struct data_proc_container *container)
{
    this->tData = container->tData;
    this->thread = thread;
    //Sql IDs dont start with 0
    this->curRow = 1;

    colHeaders = container->colHeaders;

    deleteCmd = "DELETE FROM " + tData->tableID + " WHERE ID=";
    //dealloc container bundle
    delete container;
}

std::string dataProcessor::validateFeild(std::string feild)
{
    std::string* temp;
    int i ,aviCols;
    
    Base* base = makers[tData->algorithmType]();
    aviCols = base->getColCount();
    temp = base->getPossibleFeilds(curCol);
    delete base;
    //iterate through possible values of current column
    for(i = 0; i < aviCols; i++){
        if(temp[i] == feild){
            return feild;
        }
    }

    //worst case scenario
    DEBUG_MSG(__func__, "feild does not exist:", feild);
    return "NaN";
}

std::string dataProcessor::cleanData(std::string feild)
{
    //check if empty
    if(feild.empty())
        feild = "NaN";
    else {
        //transform to lower case
        std::transform(feild.begin(), feild.end(), feild.begin(), 
        [](unsigned char c){ return std::tolower(c); });
        //check if valid feild present
        feild = validateFeild(feild);
    }
    
    return feild;
}

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
    DEBUG_MSG(__func__, "build update command for row ID:", curRow + 0);
    return updateCmd;
}

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
        sql_write(sqlUpdateCmd.c_str());
        DEBUG_MSG(__func__, "record ID:",curRow + 0," cleaned");
    }

    return 0;
}

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

int dataProcessor::deleteDuplicateRecords()
{
    std::string *rowID;
    std::string temp;

    if(selectCmd.empty())
        buildSelectCmd();
    rowID = sql_read(selectCmd.c_str(),0);
    if(!rowID){
        DEBUG_ERR(__func__, "select command read error");
        return 0;
    }
    if(!rowID->empty())
    {
        DEBUG_MSG(__func__, "cleaning up duplicate record ID:", *rowID);
        temp = deleteCmd + *rowID + ";";
        sql_write(temp.c_str());
        return 0;
    }
    else
        return 1; 
}

int process_data_start(void *data)
{
    dataProcessor *dataProc = (dataProcessor*)data;
    struct table *tData = dataProc->tData;
    std::string *feild;
    int rc;

    if(dataProc->dataCleanPhase)
    {
        rc = dataProc->deleteDuplicateRecords();
        if(rc){
            DEBUG_MSG(__func__,"All duplicate data deleted");
            tData->metadata->curCol = 0;
            return JOB_DONE;
        }
    } else {
        if(dataProc->curRow >= tData->metadata->rows)
            dataProc->dataCleanPhase = true;

        feild = get_row(tData, dataProc->curRow);
        if(feild){
            dataProc->processSql(feild);
            dataProc->curRow++;
        }
    }

    return JOB_PENDING;
}

int process_data_finalize(void *data)
{
    dataProcessor *dataProc = (dataProcessor*)data;
    struct table *tData = dataProc->tData;
    std::string selectAll = "SELECT * FROM " + tData->tableID + ";";
    std::string *getCleanedTable;

    //Send the cleaned data back to server via fwd stack
    getCleanedTable = sql_read(selectAll.c_str(), -1);
    send_packet(*getCleanedTable, tData->tableID, INTR_SEND, tData->priority);

    //Schedule the algorithm to process our cleaned data
    sched_algo(dataProc->thread, tData);

    //Deallocate memory and cleanup
    delete dataProc;

    return JOB_FINISHED;
}

struct process* data_proc = new process {
    .start_proc = process_data_start,
    .end_proc = process_data_finalize
};

int init_data_processor(struct thread_pool* thread,
                struct data_proc_container* container)
{
    dataProcessor *dpContainer = new dataProcessor(thread, container);
    scheduleTask(thread, data_proc, (void*)dpContainer, container->tData->priority);
    DEBUG_MSG(__func__, "initilized data processor");

    return 0;
}