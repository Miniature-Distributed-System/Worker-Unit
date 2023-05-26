#ifndef ALGO_H
#define ALGO_H
#include <map>
#include <string>
#include <vector>
#include "../services/sqlite_database_access.hpp"
#include "../instance/instance_list.hpp"
#include "../include/process.hpp"

//bound to change once build root is made
#define TOT_ALGO 2

struct AlgorithmPackage {
    TableData* tableData;
    std::vector<void *> resultVectors;
};

struct AlgorithmExportPackage {
    ProcessStates* proc;
    void *args;
    AlgorithmExportPackage(ProcessStates *proc, void *args){
        this->proc = proc;
        this->args = args;
    }
};

extern std::map<std::string, AlgorithmPackage *> algorithmResultMap;

int sched_algo(TableData* tableData);
int update_algo_result(std::string tableId, void *algorithmExportResult);
void dealloc_table_dat(TableData *tData);

//add function prototypes below this
AlgorithmExportPackage init_example_algo(std::string, std::string, int, int, int);
AlgorithmExportPackage init_ce_algorithm(std::string, std::string, int, int, int);

static struct AlgorithmExportPackage (*avial_algo[TOT_ALGO])(std::string tableId, std::string instanceId, int start, int end, 
    int columns) = {
        init_example_algo,
        init_ce_algorithm
};

AlgorithmExportPackage init_example_finalize(std::vector<void *>, std::string, TaskPriority);
AlgorithmExportPackage init_ce_finalize(std::vector<void *>, std::string, TaskPriority);

static struct AlgorithmExportPackage (*avialable_finalize_algo[TOT_ALGO])(std::vector<void *> resultVectors, 
    std::string tableId, TaskPriority taskPriority) = {
    init_example_finalize,
    init_ce_finalize
};

#endif