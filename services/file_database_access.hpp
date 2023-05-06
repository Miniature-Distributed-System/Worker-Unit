#ifndef DATABASE_H
#define DATABASE_H
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include "../include/flag.h"

enum FileAccessType{
    READ_FILE,
    RW_FILE
};

class FileDataBaseAccess {
        FileAccessType accessMode;
        std::list<std::string> readWriteData;
        std::vector<std::string> readData;
        std::ifstream fileAccess;
        std::string fileName;
        Flag dataModified;
        Flag fileDeleted;

    public:
        FileDataBaseAccess();
        FileDataBaseAccess(std::string fileName, FileAccessType accessMacro);
        ~FileDataBaseAccess();
        int writeBlob(std::string data);
        std::string getBlob();
        int getTotalRows();
        int getTotalColumns();
        std::vector<std::string> getRowValueList(int rowIndex);
        std::vector<std::string> getColumnNamesList();
        int writeRowValue(std::string value, int rowIndex, int columnIndex);
        int writeRowValueList(std::vector<std::string> valueList, int rowIndex);
        std::string getRowValue(int rowIndex, int columnIndex);
        int deleteDuplicateRecords(int rowIndex);
        void dropFile();
        void commitChanges();
};

#endif