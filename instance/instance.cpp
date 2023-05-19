#include <unistd.h>
#include "instance.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"

Instance::Instance(std::string instanceId, std::uint8_t linkedAlgorithm, std::uint8_t totalColumns, 
                std::uint64_t totalRows) : instanceId(instanceId), linkedAlgorithm(linkedAlgorithm),
                totalColumns(totalColumns), totalRows(totalRows)
{
    Log().info(__func__, "TableId:", this->instanceId, " totoalCols:", this->totalColumns);
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

    if(instanceList.size() == 0){
        Log().debug(__func__, "instance list is 0 therefore going to sleep for a while");
        sleep(1);
    }

    for(auto it = instanceList.begin(); it != instanceList.end(); it++){
        instanceId = (*it)->getId();
        if(!id.compare(instanceId)){
            return *(*it);
        }
    }
    Log().debug(__func__, "No match found for:", id);
    //return dummy object
    return instance;
}