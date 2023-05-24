#ifndef CE_ALGO_C
#define CE_ALGO_C
#include <vector>
#include "../include/flag.h"
#include "../include/process.hpp"
#include "../services/file_database_access.hpp"

class CandidateElimination {
    private:
        int columns;
        int targetCol;
        int totalPositives;
        int totalNegatives;
        std::vector<std::string> s;
        std::vector<std::string> g;
        std::string confirmValue;
        int start;
        int end;
    public:
        Flag startSet;
        std::string tableId;
        FileDataBaseAccess *fileDataBaseAccess;
        CandidateElimination(std::string tableId, std::string instanceId, int start, int end, int cols );
        ~CandidateElimination();
        void compare(std::vector<std::string> valueList);
        std::vector<std::string> getS() { return s; }
        std::vector<std::string> getG() { return g; }
        int getNextRow();
        std::vector<std::string> getFirstPostive();
        int getColumnsCount() { return columns; }
        int getTotalPositives(){ return totalPositives; }
        int getTotalNegatives() { return totalNegatives; }
};
#endif