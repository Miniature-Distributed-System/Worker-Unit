#ifndef DATA_PROC_H
#define DATA_PROC_H
#include "../include/process.hpp"
#include "../scheduler/thread_pool.hpp"

struct DataProcessContainer{
    std::string *colHeaders;
    struct TableData* tData;
    DataProcessContainer(std::string *colHeaders, TableData* tData){
        this->colHeaders = colHeaders;
        this->tData = tData;
    }
};

int processData(struct TableData *);
int init_data_processor(struct thread_pool* thread,
                struct DataProcessContainer* container);
#endif
