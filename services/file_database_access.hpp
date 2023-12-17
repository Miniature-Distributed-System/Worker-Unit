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

enum FileAccessErrors {
    ACCESS_DENIED           = -1,
    INVALID_ACCESS_MODE     = -2,
    DUPLICATE_CLEANUP_DONE  = -3,
    ROW_INDEX_OVERFLOW      = -4,
    COLUMN_INDEX_OVERFLOW   = -5,
    ROW_INDEX_UNDERFLOW     = -6,
    COLUMN_INDEX_UNDERFLOW  = -7,
};

class IDataContainer {
    public:
        virtual ~IDataContainer() = default;
        virtual std::string& operator[](std::size_t index) = 0;
        virtual std::size_t size() const = 0;
        virtual std::string* begin() = 0;
        virtual std::string* end() = 0;
        virtual std::string* erase(std::string *position) = 0;
        virtual std::string* insert(std::string * position, const std::string& value) = 0;
        virtual std::string& front() = 0;
        virtual void push_back(const std::string& value) = 0;
};

class VectorContainer : public IDataContainer {
    public:
        std::vector<std::string> dataContainer;
        std::string& operator[](std::size_t index) override {
            return dataContainer[index];
        }
        std::size_t size() const override {
            return dataContainer.size();
        }
        std::string* begin() override {
            return dataContainer.data();
        }
        std::string* end() override {
            return dataContainer.data() + dataContainer.size();
        }
        std::string* erase(std::string* position) override {
            auto iter = position - dataContainer.data();
            auto eraseIter = dataContainer.erase(dataContainer.begin() + iter);
            return &(*eraseIter);
        }
        std::string* insert(std::string* position, const std::string& value) override {
            auto iter = position - dataContainer.data();
            return &(*dataContainer.insert(dataContainer.begin() + iter, value));
        }
        std::string& front() override {
            return dataContainer.front();
        }
        void push_back(const std::string& value) override {
            dataContainer.push_back(value);
        }
};

class ListContainer : public IDataContainer {
    public:
        std::list<std::string> dataContainer;
        std::string& operator[](std::size_t index) override {
            auto it = std::next(dataContainer.begin(), index);
            return *it;
        }
        std::size_t size() const override {
            return dataContainer.size();
        }
        std::string* begin() override {
            return &dataContainer.front();
        }
        std::string* end() override {
            return &dataContainer.back() + 1;
        }
        std::string* erase(std::string* position) override {
            auto iter = position - &dataContainer.front();
            auto it = std::next(dataContainer.begin(), iter);
            auto eraseIter = dataContainer.erase(it);
            return &(*eraseIter);
        }
        std::string* insert(std::string* position, const std::string& value){
            auto iter = position - &dataContainer.front();
            auto it = std::next(dataContainer.begin(), iter);
            return &(*dataContainer.insert(it, value));
        }
        std::string& front() override {
            return dataContainer.front();
        }
        void push_back(const std::string& value) override {
            dataContainer.push_back(value);
        }
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