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
#include "../services/sqlite_database_access.hpp"
#include "algorithm_scheduler.hpp"
#include "candidate_elemination.hpp"
#include "ce_finalize.hpp"

CandidateElimination::CandidateElimination(std::string tableId, std::string instanceId, int start, int end, 
    int columns) : columns(columns - 1), start(start), end(end), tableId(tableId)
{
    targetCol = columns - 1;

    for(int i = 0; i < columns - 1; i++){
        s.push_back("*");
        g.push_back("*");
    }
    std::string* columnValues = sqliteDatabaseAccess->getColumnNames(instanceId, columns);
    if(columnValues){
        std::string targetColumnName = columnValues[columns - 1];
        confirmValue = sqliteDatabaseAccess->getColumnValues(instanceId, targetColumnName, 2)[0];
        if(confirmValue.empty())
            confirmValue = "yes";
    } else confirmValue = "yes";
    
    columns--;
    fileDataBaseAccess = new FileDataBaseAccess(tableId, READ_FILE);
    totalNegatives = totalPositives = 0;
    startSet.initFlag(false);
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
        totalPositives++;
        for(i = 0; i < columns; i++)
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
        totalNegatives++;
        for(i = 0; i < columns; i++)
        {
            if(!boost::iequals(s[i], valueList[i]) && !boost::iequals(g[i], "?"))
                g[i] = s[i];
            else 
                g[i] = "?";
        }
        //Log().info(__func__, "No:", getG());
    }
}

int CandidateElimination::getNextRow()
{
    if(start <= end)
        return start++;
    return -1;
}

std::vector<std::string> CandidateElimination::getFirstPostive()
{
    int startRow = start;

    while(startRow <= end){
        std::vector<std::string> feild = fileDataBaseAccess->getRowValueList(startRow++);
        if(feild.size() == columns + 1){
            if(boost::iequals(feild[targetCol], confirmValue))
                return feild;
        }
    }
    std::vector<std::string> empty;
    return empty;
}

JobStatus candidate_elimination_start(void *data)
{
    CandidateElimination *ce = (CandidateElimination*)data;
    std::vector<std::string> feild;

    if(!ce->startSet.isFlagSet()){
        feild = ce->getFirstPostive();
        if(feild.empty())
            return JOB_DONE;
        ce->compare(feild);
        ce->startSet.setFlag();
        return JOB_PENDING;
    }
    
    int rowIndex = ce->getNextRow();
    if(rowIndex < 0)
        return JOB_DONE;

    feild = ce->fileDataBaseAccess->getRowValueList(rowIndex);
    if(feild.size() == ce->getColumnsCount() + 1)
        ce->compare(feild);
    else Log().debug(__func__, "doesn't match column count :", feild.size());
    return JOB_PENDING;
}

JobStatus candidate_elimination_pause(void *data){
    Log().info(__func__, "pause process");
    return JOB_PENDING;
}

void candidate_elimination_end(void *data)
{
    CandidateElimination *ce = (CandidateElimination*)data;
    CeResultExportObject *resObj = new CeResultExportObject();

    resObj->totalColumns = ce->getColumnsCount();
    std::string str;
    for(int i = 0; i < ce->getS().size(); i++){
        str += " " + ce->getS()[i];
    }
    Log().info(__func__, "Str:",str);
    resObj->s = ce->getS();
    resObj->g = ce->getG();
    resObj->totalPositives = ce->getTotalPositives() - 1;
    resObj->totalNegatives = ce->getTotalNegatives();
    
    update_algo_result(ce->tableId, resObj);
    delete ce;
}

struct ProcessStates *ce_algorithm = new ProcessStates{
    .name = "candidate elimination",
    .type = COMPUTE_STAGE,
    .start_proc = candidate_elimination_start,
    .pause_proc = candidate_elimination_pause,
    .end_proc = candidate_elimination_end
};

AlgorithmExportPackage init_ce_algorithm(std::string tableId, std::string instanceId, int start, int end, int columns)
{
    CandidateElimination *ce = new CandidateElimination(tableId, instanceId, start, end, columns);
    AlgorithmExportPackage finalizePackage(ce_algorithm, ce);
    return finalizePackage;
}