#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <array>
#include "../include/process.hpp"
#include "../include/task.hpp"
#include "../include/debug_rp.hpp"
#include "../include/packet.hpp"
#include "../include/logger.hpp"
#include "../sender_proc/sender.hpp"
#include "../instance/instance_list.hpp"
#include "algorithm_scheduler.hpp"
#include "candidate_elemination.hpp"

CandidateElimination::CandidateElimination(int columns, TableData *tableData)
{
    this->tableName = tableData->tableID;
    cols = columns - 1;
    targetCol = columns - 1;
    s = new std::string[cols];
    g = new std::string[cols];

    for(int i = 0; i < cols; i++){
        s[i] = "*";
        g[i] = "?";
    }
    std::string* columnValues = sqliteDatabaseAccess->getColumnNames(tableData->instanceType, columns);
    if(columnValues){
        std::string targetColumnName = columnValues[columns - 1];
        confirmValue = sqliteDatabaseAccess->getColumnValues(tableData->instanceType, targetColumnName, 2)[0];
        if(confirmValue.empty())
            confirmValue = "yes";
    } else confirmValue = "yes";
    
    fileDataBaseAccess = new FileDataBaseAccess(tableName, READ_FILE);
}

CandidateElimination::~CandidateElimination()
{
    delete fileDataBaseAccess;
}

void CandidateElimination::compare(std::vector<std::string> valueList)
{
    int i;
    
    //positive training examples
    //Log().info(__func__, input[targetCol]);
    if(boost::iequals(valueList[targetCol], confirmValue))
    {
        for(i = 0; i < cols; i++)
        {
            if(s[i] == "*")
                s[i] = valueList[i]; 
            else if(!boost::iequals(s[i], valueList[i])){
                s[i] = "?";
                g[i] = "?";
            }
        }
        //Log().info(__func__, "Yes:", getS());
    //negative training examples
    } else {
        for(i = 0; i < cols; i++)
        {
            if(!boost::iequals(s[i], valueList[i]))
                g[i] = s[i];
        }
        //Log().info(__func__, "No:", getG());
    }
}

std::string CandidateElimination::getS()
{
    int i;
    std::string finalStr;
    
    for(i = 0; i < cols; i++){
        finalStr += s[i];
        if(i < cols -1)
            finalStr += ",";
    }

    return finalStr;
}

std::string CandidateElimination::getG()
{
    int i;
    std::string finalStr;
    
    for(i = 0; i < cols; i++){
        finalStr += g[i];
        if(i < cols -1)
            finalStr += ",";
    }

    return finalStr;
}

JobStatus candidate_elimination_start(void *data)
{
    TableData* tData = (TableData*)data;
    CandidateElimination *ce = (CandidateElimination*)tData->args;
    std::vector<std::string> feild;
    
    if(tData->metadata->currentRow >= tData->metadata->rows)
        return JOB_DONE;
    feild = ce->fileDataBaseAccess->getRowValueList(tData->metadata->currentRow);
    if(feild.size() == tData->metadata->columns)
        ce->compare(feild);
    else Log().error(__func__, "doesn't match column count :", feild.size());
    tData->metadata->currentRow++;
    return JOB_PENDING;
}

JobStatus candidate_elimination_pause(void *data){
    Log().info(__func__, "pause process");
    return JOB_PENDING;
}

JobStatus candidate_elimination_end(void *data, JobStatus status)
{
    TableData* tData = (TableData*)data;
    CandidateElimination *ce = (CandidateElimination*)tData->args;
    std::string s = ce->getS();
    std::string g = ce->getG();
    std::string final = s + ":" + g;
    send_packet(final, tData->tableID, FRES_SEND, tData->priority);
    Log().info(__func__, "end process S:",s, " G:", g, " for table:", tData->tableID); 
    delete ce;
    //Deallocate both table data and whatever was allocated in this algo before
    //winding up with the process, else we will leak memeory.
    instanceList.dereferenceInstance(tData->tableID);
    dealloc_table_dat(tData);

    return JOB_FINISHED;
}

struct ProcessStates *ce_algorithm = new ProcessStates{
    .start_proc = candidate_elimination_start,
    .pause_proc = candidate_elimination_pause,
    .end_proc = candidate_elimination_end
};

ProcessStates* init_ce_algorithm(TableData* tData)
{
    CandidateElimination *ce = new CandidateElimination(tData->metadata->columns, tData);
    tData->args = ce;
    return ce_algorithm;
}
