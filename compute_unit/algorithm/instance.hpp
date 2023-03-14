#ifndef INSTANCE_H
#define INSTANCE_H

#include <list>
#include <string>

class Instance {
        std::string instanceId;
        std::uint8_t linkedAlgorithm;
        std::uint8_t totalColumns;
        std::uint8_t totalRows;
    public:
        Instance(){
            instanceId = "";
            linkedAlgorithm = 0;
            totalColumns = 0;
            totalRows = 0;
        }
        Instance(std::string instanceId, std::uint8_t linkedAlgorithm, std::uint8_t total_columns, 
                        std::uint64_t totalRows);
        std::string getId();
        std::uint8_t getLinkedAlgorithm();
        std::uint8_t getTotalColumns();
        std::uint64_t getTotalRows();
};

class InstanceList {
        std::list<Instance*> instanceList;
    public:
        void addInstance(Instance*);
        Instance getInstanceFromId(std::string id);
};

extern InstanceList globalInstanceList;

#endif