#ifndef PROC_H
#define PROC_H
#include <string>

struct process {
    int (*start_proc)(void*);
    int (*pause_proc)(void*);
    int (*end_proc)(void*);
};

struct process_table {
    struct process *proc;
    void *args;
    std::uint8_t priority;
    std::uint16_t starveCounter;
};

struct table_metadata {
    std::uint64_t rows;
    std::uint64_t cols;
    std::uint64_t curRow;
    std::uint64_t curCol;
};

struct table {
    struct table_metadata *metadata;
    void* args;
    std::string tableID;
    unsigned int dataLen;
    TaskPriority priority;
    std::uint8_t algorithmType;

    table(std::uint64_t row, std::uint64_t col){
        metadata = new table_metadata;
        metadata->cols = col;
        metadata->rows = row;
        metadata->curCol = 0;
        metadata->curRow = 0;
    }

    ~table(){
        delete metadata;
    }
};

int scheduleTask(struct thread_pool *thread, struct process* proc, 
                void *args, int priority);

#endif