#ifndef DATA_PROC_H
#define DATA_PROC_H
#include "../include/process.hpp"
#include "../scheduler/thread_pool.hpp"

struct DataProcessContainer{
    std::string *colHeaders;
    TableData* tData;
    DataProcessContainer(){};
    DataProcessContainer(std::string *colHeaders, TableData* tData) : colHeaders(colHeaders), tData(tData) {};
};

int processData(TableData *);
int init_data_processor(ThreadPool* thread, DataProcessContainer container);
#endif
