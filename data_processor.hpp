#ifndef DATA_PROC_H
#define DATA_PROC_H
#include "include/process.hpp"
#include "thread_pool.hpp"

struct DataProcessContainer{
    std::string *colHeaders;
    struct table* tData;
    DataProcessContainer(std::string *colHeaders, table* tData){
        this->colHeaders = colHeaders;
        this->tData = tData;
    }
};

int processData(struct table *);
int init_data_processor(struct thread_pool* thread,
                struct DataProcessContainer* container);
#endif
