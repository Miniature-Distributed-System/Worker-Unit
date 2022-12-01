#ifndef DATA_PROC_H
#define DATA_PROC_H
#include<string>

struct table_metadata {
    unsigned int rows;
    unsigned int cols;
};

struct table {
    struct table_metadata *metadata;
    std::string tableID;
    std::string firstColName;
    unsigned int dataLen;
    short priority;
    short algorithmType;
};

int processData(struct table *);

#endif
