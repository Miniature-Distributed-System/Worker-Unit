#include <cstdlib>
#include "../include/debug_rp.hpp"
#include "../sql_access.hpp"
#include "instance_list.hpp"

DataBaseInstanceList::DataBaseInstanceList()
{
    sem_init(&instanceListLock, 0, 1);
}

std::string getPrefixString()
{
    int requiredStringLength = 5;
    int maxAlphabets = 27;
    std::string alphabetPool = "abcdefghijklmnopqrstuvwxyz";
    std::string finalString;
    for(int i = 0; i < requiredStringLength; i++){
        finalString += alphabetPool[rand() % maxAlphabets];
    }

    return finalString;
}

void deleteInstanceModel(InstanceModel *instanceModel)
{
    std::string aliasName = instanceModel->aliasName;
    std::string dropTableQuery = "DROP TABLE " + aliasName + ";";
    dataBaseAccess->writeValue(dropTableQuery.c_str());
    delete instanceModel;

    DEBUG_MSG(__func__, "deallocated and delete instance model:", aliasName);
}

std::string DataBaseInstanceList::addInstance(std::string instanceName)
{
    std::string aliasName = instanceName + "_" + getPrefixString();
    InstanceModel *instanceModel = new InstanceModel(instanceName, aliasName);

    sem_wait(&instanceListLock);
    for(auto i = instanceList.begin(); i != instanceList.end(); i++){
        InstanceModel *iteratorInstanceModel = (*i);
        if(!iteratorInstanceModel->actualName.compare(instanceName)){
            if(iteratorInstanceModel->scheduledToDelete.isFlagSet()) continue;
            if(iteratorInstanceModel->referenceCount == 0){
                deleteInstanceModel(iteratorInstanceModel);
                instanceList.erase(i--);
            } else {
                iteratorInstanceModel->scheduledToDelete.setFlag();
                DEBUG_MSG(__func__,"Table with alias:", iteratorInstanceModel->aliasName, " is scheduled for deletion");
            }
        }
    }

    instanceList.push_back(instanceModel);
    sem_post(&instanceListLock);
    DEBUG_MSG(__func__, "Added instance:", instanceName, " into database as:", aliasName);
    return aliasName;
}

void DataBaseInstanceList::incrimentRefrence(std::string instanceName)
{
    sem_wait(&instanceListLock);
    for(auto i = instanceList.begin(); i != instanceList.end(); i++){
        InstanceModel *iteratorInstanceModel = (*i);
        if(!iteratorInstanceModel->actualName.compare(instanceName)){
            // Only if instance is not scheduled for deletion
            if(!iteratorInstanceModel->scheduledToDelete.isFlagSet()){
                iteratorInstanceModel->referenceCount++;
                DEBUG_MSG(__func__, "reference count:",iteratorInstanceModel->referenceCount, " currently for ",
                                iteratorInstanceModel->aliasName);
                break;
            }
        }
    }
    sem_post(&instanceListLock);
}

void DataBaseInstanceList::dereferenceInstance(std::string aliasName)
{
    sem_wait(&instanceListLock);
    for(auto i = instanceList.begin(); i != instanceList.end(); i++){
        InstanceModel *iteratorInstanceModel = (*i);
        if(!iteratorInstanceModel->aliasName.compare(aliasName)){
            // Only if instance is not scheduled for deletion
            if(!iteratorInstanceModel->scheduledToDelete.isFlagSet()){
                iteratorInstanceModel->referenceCount--;
                DEBUG_MSG(__func__, "reference count:",iteratorInstanceModel->referenceCount, " currently for ",
                                iteratorInstanceModel->aliasName);
                break;
            }
        }
    }
    sem_post(&instanceListLock);
}

std::string DataBaseInstanceList::getInstanceActualName(std::string aliasName)
{
    for(auto i = instanceList.begin(); i != instanceList.end(); i++){
        InstanceModel *iteratorInstanceModel = (*i);
        if(!iteratorInstanceModel->aliasName.compare(aliasName)){
            return iteratorInstanceModel->actualName;
        }
    }

    DEBUG_ERR(__func__, "failed to fetch the alias name of table:", aliasName, " from list");
    return "";
}

std::string DataBaseInstanceList::getInstanceAliasName(std::string instanceName)
{
    for(auto i = instanceList.begin(); i != instanceList.end(); i++){
        InstanceModel *iteratorInstanceModel = (*i);
        if(!iteratorInstanceModel->actualName.compare(instanceName)){
            return iteratorInstanceModel->aliasName;
        }
    }

    DEBUG_ERR(__func__, "failed to fetch the alias for instance:", instanceName, " from list");
    return "";
}