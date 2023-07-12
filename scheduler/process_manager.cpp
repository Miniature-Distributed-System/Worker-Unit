#include <random>
#include "../include/flag.h"
#include "../include/process.hpp"
#include "../include/logger.hpp"
#include "process_manager.hpp"

ProcessTable* ProcessManager::registerProcess(ProcessStates *process, void *args, TaskPriority prior)
{
    Flag exitLoop;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(1, 9999);
    std::uint64_t pid;

    exitLoop.initFlag(false);

    while(exitLoop.isFlagSet()){
        pid = distr(gen);
        exitLoop.setFlag();
        for(auto iterator = pidVector.begin(); iterator != pidVector.end(); iterator++){
            if(pid == (*iterator)->pid){
                exitLoop.resetFlag();
                break;
            }
        }
    }

    ProcessTable* procTable = new ProcessTable();
    procTable->pid = pid;
    pidVector.push_back(procTable);
}

void ProcessManager::unregisterProcess(ProcessTable *processTable)
{
    for(auto it = pidVector.begin(); it != pidVector.end(); it++){
        if(*it == processTable){
            delete processTable;
            pidVector.erase(it);
        }
    }
}