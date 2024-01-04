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
        virtual std::size_t size() const = 0;
        virtual std::string& front() = 0;
        virtual void push_back(const std::string& value) = 0;
		virtual std::string getRow(int) = 0;
		virtual std::string getBlob() = 0;
		virtual void replaceRow(int, std::string) = 0;
		virtual void deleteRow(int) = 0;
};

class VectorContainer : public IDataContainer {
		std::vector<std::string> dataContainer;
	public:
		std::size_t size() const override;
        std::string& front() override;
        void push_back(const std::string& value) override;
        std::string getRow(int index) override;
        std::string getBlob() override;
        void replaceRow(int index, std::string rowValue) override;
        void deleteRow(int index) override;
};

class ListContainer : public IDataContainer {
		std::vector<std::string> dataContainer;
	public:
		std::size_t size() const override;
        std::string& front() override;
        void push_back(const std::string& value) override;
        std::string getRow(int index) override;
        std::string getBlob() override;
        void replaceRow(int index, std::string rowValue) override;
        void deleteRow(int index) override;
};

class FileDataBaseAccess {
        FileAccessType accessMode;
        ListContainer readWriteData;
        VectorContainer readData;
        std::fstream fileAccess;
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

        IDataContainer& getContainer();
};

#endif
