#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <array>
#include "algo.hpp"
#include "../include/process.hpp"
#include "../include/task.hpp"
#include "../sender_proc/sender.hpp"
#include "../include/debug_rp.hpp"
#include "../instance/instance_list.hpp"
//This needs nuking as dirs will change later
#include "../include/packet.hpp"
#include "cand_elim.hpp"

CandidateElimination::CandidateElimination(int n)
{
    cols = n - 2;
    targetCol = n - 1;
    s = new std::string[cols];
    g = new std::string[cols];
    //DEBUG_MSG(__func__, "cols", cols, "target",targetCol);
    for(int i = 0; i < cols; i++){
        s[i] = "*";
        g[i] = "?";
    }
    // dataBaseAccess = new DatabaseAccess();
    // dataBaseAccess->initDatabase();
}

void CandidateElimination::compare(std::string *input)
{
    int i;
    
    //positive training examples
    //DEBUG_MSG(__func__, input[targetCol]);
    if(boost::iequals(input[targetCol], True))
    {
        for(i = 0; i < cols; i++)
        {
            if(s[i] == "*")
                s[i] = input[i]; 
            else if(!boost::iequals(s[i], input[i])){
                s[i] = "?";
                g[i] = "?";
            }
        }
        //DEBUG_MSG(__func__, "Yes:", getS());
    //negative training examples
    } else {
        for(i = 0; i < cols; i++)
        {
            if(!boost::iequals(s[i], input[i]))
                g[i] = s[i];
        }
        //DEBUG_MSG(__func__, "No:", getG());
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

std::string *CandidateElimination::getValidationRow(int row)
{
    std::string *temp = new std::string[3];
    for(int i = 0; i < 3; i++){
        temp[i] = validationData[row][i];
    }
    return temp;
}

JobStatus candidate_elimination_start(void *data)
{
    TableData* tData = (TableData*)data;
    CandidateElimination *ce = (CandidateElimination*)tData->args;
    std::string *feild;
    
    if(tData->metadata->currentRow >= tData->metadata->rows)
        return JOB_DONE;
    feild = dataBaseAccess->getRowValues(tData, tData->metadata->currentRow);
    if(feild)
        ce->compare(feild);
    tData->metadata->currentRow++;
    return JOB_PENDING;
}

JobStatus candidate_elimination_pause(void *data){
    DEBUG_MSG(__func__, "pause process");
    return JOB_PENDING;
}

JobStatus candidate_elimination_end(void *data, JobStatus status)
{
    TableData* tData = (TableData*)data;
    CandidateElimination *ce = (CandidateElimination*)tData->args;
    std::string s = ce->getS();
    std::string g = ce->getG();
    std::string final = s + "\n" + g;
    send_packet(final, tData->tableID, FRES_SEND, tData->priority);
    DEBUG_MSG(__func__, "end process S:",s, " G:", g, " for table:", tData->tableID);
    instanceList.dereferenceInstance(tData->tableID);
    dealloc_table_dat(tData);
    //Deallocate both table data and whatever was allocated in this algo before
    //winding up with the process, else we will leak memeory.
    delete ce;

    return JOB_FINISHED;
}

struct ProcessStates *ce_algorithm = new ProcessStates{
    .start_proc = candidate_elimination_start,
    .pause_proc = candidate_elimination_pause,
    .end_proc = candidate_elimination_end
};

ProcessStates* init_ce_algorithm(TableData* tData)
{
    CandidateElimination *ce = new CandidateElimination(tData->metadata->columns);
    tData->args = ce;
    return ce_algorithm;
}
