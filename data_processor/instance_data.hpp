#ifndef INSTANCE_DATA_H
#define INSTANCE_DATA_H
#include <string>
#include <vector>
#include "../algorithm/algorithm_scheduler.hpp"
#include "../instance/instance_list.hpp"
#include "../instance/instance.hpp"

class InstanceData {
        Instance instance;
        std::vector<std::string*> instanceDataMatrix;
        std::string userTableName;
        int userTableTotalColumns;
        std::string errorString;
        FileDataBaseAccess *fileDataBaseAccess;
    public:
        InstanceData(Instance instance, int columns, std::string userTableId);
        ~InstanceData();
        int initlizeData();
        std::string* validateColumns();
        std::string* getPossibleFields(int column);
        std::uint64_t getTotalRows() { return instance.getTotalRows(); }
        std::string getErrorString() { return errorString;}
};

#endif