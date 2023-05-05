#include <string>
#include <vector>
#include <array>
#include "algo.hpp"
#include "../sql_access.hpp"
#include "../include/process.hpp"
#include "../include/task.hpp"
#include "../sender_proc/sender.hpp"
#include "../include/debug_rp.hpp"
//This needs nuking as dirs will change later
#include "../include/packet.hpp"
#include "cand_elim.hpp"

candidateElimination::candidateElimination(int n)
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
}

void candidateElimination::compare(std::string *input)
{
    int i;
    
    //positive training examples
    //DEBUG_MSG(__func__, input[targetCol]);
    if(input[targetCol] == True)
    {
        for(i = 0; i < cols; i++)
        {
            if(s[i] == "*")
                s[i] = input[i]; 
            else if(s[i] != input[i]){
                s[i] = "?";
                g[i] = "?";
            }
        }
        //DEBUG_MSG(__func__, "Yes:", getS());
    //negative training examples
    } else {
        for(i = 0; i < cols; i++)
        {
            if(s[i] != input[i])
                g[i] = s[i];
        }
        //DEBUG_MSG(__func__, "No:", getG());
    }
}

std::string candidateElimination::getS()
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

std::string candidateElimination::getG()
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

std::string *candidateElimination::getValidationRow(int row)
{
    std::string *temp = new std::string[3];
    for(int i = 0; i < 3; i++){
        temp[i] = validationData[row][i];
    }
    return temp;
}

JobStatus candidate_elimination_start(void *data)
{
    struct table* tData = (struct table*)data;
    candidateElimination *ce = (candidateElimination*)tData->args;
    std::string *feild;
    
    if(tData->metadata->curRow >= tData->metadata->rows)
        return JOB_DONE;
    feild = dataBaseAccess->getRowValues(tData, tData->metadata->curRow);
    if(feild)
        ce->compare(feild);
    tData->metadata->curRow++;
    return JOB_PENDING;
}

JobStatus candidate_elimination_pause(void *data){
    DEBUG_MSG(__func__, "pause process");
    return JOB_PENDING;
}

JobStatus candidate_elimination_end(void *data, JobStatus status)
{
    struct table* tData = (struct table*)data;
    candidateElimination *ce = (candidateElimination*)tData->args;
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

struct process *ce_algorithm = new process{
    .start_proc = candidate_elimination_start,
    .pause_proc = candidate_elimination_pause,
    .end_proc = candidate_elimination_end
};

struct process* init_ce_algorithm(struct table* tData)
{
    candidateElimination *ce = new candidateElimination(tData->metadata->cols);
    tData->args = ce;
    return ce_algorithm;
}
