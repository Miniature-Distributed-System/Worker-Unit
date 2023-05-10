#ifndef CE_ALGO_C
#define CE_ALGO_C
#include "base_class.hpp"
#include "../services/sqlite_database_access.hpp"
#include "../services/file_database_access.hpp"

class CandidateElimination {
    private:
        int cols;
        int targetCol;
        std::string *s;
        std::string *g;
        std::string True = "yes";
        std::string tableName;
    public:
        FileDataBaseAccess *fileDataBaseAccess;
        CandidateElimination(int cols, std::string tableName);
        ~CandidateElimination();
        CandidateElimination(int n);
        void compare(std::vector<std::string> valueList);
        std::string getS();
        std::string getG();
};
#endif