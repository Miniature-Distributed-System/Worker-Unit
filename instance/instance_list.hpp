#ifndef INSTANCE_LIST_H
#define INSTANCE_LIST_H

#include <list>
#include <semaphore.h>
#include "instance_model.hpp"

class DataBaseInstanceList {
        sem_t instanceListLock;
        std::list<InstanceModel*> instanceList;
    public:
        DataBaseInstanceList();
        std::string addInstance(std::string instanceName);
        void incrimentRefrence(std::string instanceName);
        void dereferenceInstance(std::string aliasName);
        std::string getInstanceActualName(std::string aliasName);
        std::string getInstanceAliasName(std::string instanceName);
};

extern DataBaseInstanceList instanceList;

#endif