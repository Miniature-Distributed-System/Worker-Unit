#ifndef CE_ALGO_C
#define CE_ALGO_C
#include "base_class.hpp"
#include "../sql_access.hpp"

class candidateElimination : public Base {
    private:
        int cols;
        static const int ValidationCols = 3;
        static const int ValidationRows = 7;
        int targetCol;
        std::string *s;
        std::string *g;
        std::string True = "yes";
        std::string validationData[ValidationRows][ValidationCols] = 
                     {{"sunny","rainy","cloudy"},
                      {"warm","cold","normal"},
                      {"normal","high",""},
                      {"strong","weak",""},
                      {"warm","same","cool"},
                      {"same","change",""},
                      {"yes","no",""}};
        std::string *getValidationRow(int row);
        int colCount(){
            return ValidationCols;
        }
        int rowCount(){
            return ValidationRows;
        }

    public:
        //DatabaseAccess *dataBaseAccess;
        candidateElimination(){};
        candidateElimination(int n);
        void compare(std::string *input);
        std::string getS();
        std::string getG();
};
#endif