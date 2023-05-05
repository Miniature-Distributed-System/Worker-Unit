#include "instance.hpp"
#include "../include/debug_rp.hpp"
Instance::Instance(std::string instanceId, std::uint8_t linkedAlgorithm, std::uint8_t totalColumns, 
                std::uint64_t totalRows)
{
    this->instanceId = instanceId;
    this->linkedAlgorithm = linkedAlgorithm;
    this->totalColumns = totalColumns;
    this->totalRows = totalRows;
    DEBUG_MSG(__func__, "TableId:", this->instanceId, " totoalCols:", this->totalColumns + 0);
}

std::string Instance::getId()
{
    return instanceId;
}

std::uint8_t Instance::getLinkedAlgorithm()
{
    return linkedAlgorithm;
}

std::uint8_t Instance::getTotalColumns()
{
    return totalColumns;
}

std::uint64_t Instance::getTotalRows()
{
    return totalRows;
}

void InstanceList::addInstance(Instance *instance)
{
    instanceList.push_back(instance);
}

Instance InstanceList::getInstanceFromId(std::string id)
{
    Instance instance;
    std::string instanceId;
    for(auto it = instanceList.begin(); it != instanceList.end(); it++){
        instanceId = (*it)->getId();
        if(id == instanceId){
            return *(*it);
        }
    }
    DEBUG_ERR(__func__, "No match found");
    //return dummy object
    return instance;
}