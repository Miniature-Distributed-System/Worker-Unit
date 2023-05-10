#ifndef SQL_ACC_H
#define SQL_ACC_H
#include <sqlite3.h>
#include <string>

class SqliteDatabaseAccess {
        sqlite3 *db;
    public:
        ~SqliteDatabaseAccess();
        int initDatabase();
        std::string* getRowValues(struct TableData* tData, int rowNumber);
        std::string* readValue(const char* sqlQuery, int column);
        std::string* getColumnNames(std::string tableID, int cols);
        std::string* getColumnValues(std::string tableName, std::string columnName, int rows);
        int writeValue(const char * sqlQuery);
};

extern SqliteDatabaseAccess *sqliteDatabaseAccess;

#endif