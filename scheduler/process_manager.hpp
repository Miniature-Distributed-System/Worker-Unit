#ifndef _PROC_MANAGER_H
#define _PROC_MANAGER_H
#include <vector>
#include "../include/process.hpp"
#include "../services/global_objects_manager.hpp"

class ProcessManager : public Base {
        std::vector<ProcessTable*> pidVector;
    public:
		static const std::string& getId()
        {
            static const std::string id = "ProcessManager";
            return id;
        }
        ProcessTable* registerProcess();
        void unregisterProcess(ProcessTable *process);       
};

#endif
