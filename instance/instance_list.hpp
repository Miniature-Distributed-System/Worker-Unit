#ifndef INSTANCE_LIST_H
#define INSTANCE_LIST_H

#include <list>
#include <semaphore.h>
#include "instance_model.hpp"
#include "../services/global_objects_manager.hpp"

class DataBaseInstanceList : public Base {
        sem_t instanceListLock;
        std::list<InstanceModel*> instanceList;
    public:
        DataBaseInstanceList();
		static const std::string& getId()
        {
            static const std::string id = "InstanceList";
            return id;
        }
        std::string addInstance(std::string instanceName);
        void incrimentRefrence(std::string instanceName);
        void dereferenceInstance(std::string aliasName);
        std::string getInstanceActualName(std::string aliasName);
        std::string getInstanceAliasName(std::string instanceName);
};

#endif
