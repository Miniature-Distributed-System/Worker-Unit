#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include "../include/process.hpp"
#include "../include/logger.hpp"
#include "../services/file_database_access.hpp"
#include "../sender_proc/sender.hpp"
#include "data_tracker.hpp"
#include "instance_data.hpp"
#include "data_validator.hpp"

class ValidateData {
        InstanceData *instanceData;
        FileDataBaseAccess *fileDataBaseAccess;
        int startRow;
        int endRow;
        int iterator;
        int curCol;
        int curRow;
    public:
        TableData *tableData;
        ValidateData(int startIndex, int endIndex, TableData *tableData, InstanceData *instanceData);
        ~ValidateData(){ delete fileDataBaseAccess; }
        std::string validateFeild(std::string feild);
        std::string cleanData(std::string feild);
        int processFeild();
};


ValidateData::ValidateData(int startIndex, int endIndex, TableData *tableData, InstanceData *instanceData) : startRow(startIndex), endRow(endIndex), 
    tableData(tableData), instanceData(instanceData)
{
    iterator = startRow;
    fileDataBaseAccess = new FileDataBaseAccess(tableData->tableID, RW_FILE);
}

// validateFeild(): This is used by cleanData() to valdiate each field with the possible set of values.
std::string ValidateData::validateFeild(std::string feild)
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
    Log().dataProcInfo(__func__, "feild does not exist:", feild);
    return "NaN";
}

// cleanData(): This method is used by the processSql() to replace invalid data fields with NaN.
std::string ValidateData::cleanData(std::string feild)
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
int ValidateData::processFeild()
{
    int i, cols = tableData->metadata->columns, rc;
    std::vector<std::string> temp;
    std::string sqlUpdateCmd;

    if(iterator >= tableData->metadata->rows)
        return 1;

    std::vector<std::string> feildList = fileDataBaseAccess->getRowValueList(iterator);
    curCol = 0;
    for(i = 0; i < cols; i++,curCol++){
        std::string str = cleanData(feildList[i]);
        temp.push_back(str);
    }
    if(temp != feildList){
        Log().dataProcInfo(__func__, "row:", iterator);
        fileDataBaseAccess->writeRowValueList(temp, iterator);
    }

    iterator++;
    return 0;
}

JobStatus validate_data_start(void *data)
{
    ValidateData *cleanData = (ValidateData*)data;
    
    if(cleanData->processFeild())
        return JOB_DONE;

    return JOB_PENDING;
}

JobStatus validate_data_pause(void *data)
{
    ValidateData *cleanData = (ValidateData*)data;
    if(cleanData)
        senderSink.pushPacket("", cleanData->tableData->tableID, RESET_TIMER, cleanData->tableData->priority);
    return JOB_PENDING;
}

void validate_data_finalize(void *data)
{
    ValidateData *cleanData = (ValidateData*)data;

    update_clean_stages(cleanData->tableData);
    if(!cleanData){
        Log().error(__func__, "CleanData object is null");
    } else Log().dataProcInfo(__func__, "end validate phase");
    delete cleanData;
}


struct ProcessStates* valdiate_data_process = new ProcessStates {
    .name = "unsplit data processor",
    .type = DATAPROCESSOR_STAGE,
    .start_proc = validate_data_start,
    .pause_proc = validate_data_pause,
    .end_proc = validate_data_finalize
};

void init_validate_data_phase(int startIndex, int endIndex, TableData *tableData, InstanceData *instanceData)
{
    ValidateData *cleanData = new ValidateData(startIndex, endIndex, tableData, instanceData);
    if(scheduleTask(valdiate_data_process, cleanData, tableData->priority))
        Log().error(__func__, "failed to schedule task pipeline broken for:", tableData->tableID);
    Log().dataProcInfo(__func__, "Data cleaning phase initilized and scheduled for:", tableData->tableID);
}