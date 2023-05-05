#ifndef BASE_C
#define BASE_C
#include <string>

class Base {
    private:
        virtual std::string* getValidationRow(int row){
            return NULL;
        }
        virtual int colCount(){
            return 0;
        }
        virtual int rowCount(){
            return 0;
        }
    public:
        std::string* getPossibleFeilds(int row){
            return getValidationRow(row);
        }
        int getColCount(){
            return colCount();
        }
        int getRowCount(){
            return rowCount();
        }
};

#endif