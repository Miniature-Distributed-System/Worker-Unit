#include <string>
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../include/packet.hpp"
#include "../include/logger.hpp"
#include "../sender_proc/sender.hpp"
#include "algorithm_scheduler.hpp"
#include "ce_finalize.hpp"

CeFinalize::CeFinalize(std::vector<void*> resultVectors, std::string tableId, TaskPriority priority) : 
    resultVectors(resultVectors), tableId(tableId), taskPriority(priority)
{
    totalPositves = totalNegatives = 0;
}

void CeFinalize::aggrigateResults()
{
    bool start = false;
    for(int i = 0; i < resultVectors.size(); i++){
        CeResultExportObject *res = (CeResultExportObject*)resultVectors[i];
        totalPositves += res->totalPositives;
        totalNegatives += res->totalNegatives;
        if(start == false){
            s = res->s;
            g = res->g;
            if(s[0].compare("*"))
                start = true;
        } else {
            for(int j = 0; j < res->totalColumns; j++){
                if(res->s[j].compare(s[j]))
                    s[j] = "?";
                if(res->g[j].compare(g[j]))
                    g[j] = "?";
            }
        }
        // std::string str;
        // for(int j = 0; j < res->totalColumns; j++){
        //     str += s[j] + ",";
        // }
        // str += ":";
        // for(int j = 0; j < res->totalColumns; j++){
        //     str += g[j] + ",";
        // }
        // Log().info(__func__, "current stage res:", str);
    }
}

std::string CeFinalize::getFinalResult()
{
    std::string finalResult;
    for(int i = 0; i < s.size() - 1; i++){
        finalResult += s[i] + ",";
    }
    finalResult += s[s.size() - 1] + ":";

    for(int i = 0; i < g.size() - 1; i++){
        finalResult += g[i] + ",";
    }
    finalResult += g[g.size() - 1] + ":";

    finalResult += std::to_string(totalPositves) + ":" + std::to_string(totalNegatives);

    return finalResult;
}

JobStatus ce_finalize_start_process(void *data)
{
    if(!data)
        JOB_FAILED;
    CeFinalize *ce_finalize = (CeFinalize*)data;
    ce_finalize->aggrigateResults();
    return JOB_DONE;
}

JobStatus ce_finalize_pause_process(void *data){
    
    return JOB_DONE;
}

void ce_finalize_end_process(void *data)
{
    CeFinalize *ce_finalize = (CeFinalize*)data;
    std::string finalString = ce_finalize->getFinalResult();
    Log().info(__func__, "final result is:", finalString);
    send_packet(finalString, ce_finalize->tableId, FRES_SEND, ce_finalize->taskPriority);
    delete ce_finalize;
}

struct ProcessStates* ce_finalize_proc = new ProcessStates{
    .start_proc = ce_finalize_start_process,
    .pause_proc = ce_finalize_pause_process,
    .end_proc = ce_finalize_end_process
};

AlgorithmExportPackage init_ce_finalize(std::vector<void*> resultVectors, std::string tableId, TaskPriority priority)
{
    CeFinalize *ce_finalize = new CeFinalize(resultVectors, tableId, priority);
    return AlgorithmExportPackage(ce_finalize_proc, ce_finalize);
}