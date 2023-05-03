#ifndef INSTANCE_MODEL_H
#define INSTANCE_MODEL_H
#include <string>
#include "../include/flag.h"

class InstanceModel {
    public:
        InstanceModel(std::string actualName, std::string aliasName){
            this->actualName = actualName;
            this->aliasName = aliasName;
            scheduledToDelete.initFlag(false);
            referenceCount = 0;
        };
        std::string aliasName;
        std::string actualName;
        Flag scheduledToDelete;
        std::uint64_t referenceCount;
};

#endif