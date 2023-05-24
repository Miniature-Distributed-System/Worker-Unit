#ifndef _CE_POSITIVES_H
#define _CE_POSITIVES_H
#include <string>
#include <vector>
#include "../include/process.hpp"
struct CeResultExportObject {
    int totalColumns;
    int totalPositives;
    int totalNegatives;
    std::vector<std::string> s;
    std::vector<std::string> g;
    void dealloc() { delete this; }
};

class CeFinalize {
        int totalPositves;
        int totalNegatives;
        std::vector<void*> resultVectors;
        std::vector<std::string> s;
        std::vector<std::string> g;
    public:
        std::string tableId;
        TaskPriority taskPriority;
        CeFinalize(std::vector<void*> resultVectors, std::string tableId, TaskPriority taskPriority);
        void aggrigateResults();
        std::string getFinalResult();
};

#endif