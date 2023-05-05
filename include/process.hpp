#ifndef PROC_H
#define PROC_H
#include <string>
#include "task.hpp"

struct ProcessStates {
    JobStatus (*start_proc)(void*);
    JobStatus (*pause_proc)(void*);
    JobStatus (*end_proc)(void*, JobStatus);
};

struct TaskData {
    struct ProcessStates *proc;
    void *args;
    std::uint8_t priority;
    std::uint16_t starveCounter;
    TaskData(){
        proc = NULL;
        args = NULL;
    };
};

struct TableMetadata {
    std::uint64_t rows;
    std::uint64_t cols;
    std::uint64_t curRow;
    std::uint64_t curCol;
};

struct TableData {
    struct TableMetadata *metadata;
    void* args;
    std::string tableID;
    unsigned int dataLen;
    TaskPriority priority;
    std::string instanceType;

    TableData(std::uint64_t row, std::uint64_t col){
        metadata = new TableMetadata;
        metadata->cols = col;
        metadata->rows = row;
        metadata->curCol = 0;
        metadata->curRow = 0;
    }

    ~TableData(){
        delete metadata;
    }
};

int scheduleTask(struct thread_pool *thread, struct ProcessStates* proc, 
                void *args, int priority);

#endif