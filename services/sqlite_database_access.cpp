#include <list>
#include "sqlite_database_access.hpp"
#include "../include/debug_rp.hpp"
#include "../data_processor/data_processor.hpp"
#include "../include/logger.hpp"

static int colNum;

/* initDatabase(): This method is used to setup/initlize the sqlite3 database, for whatever reason we can only
* create a single instance of this database accessor or access point(non of the mehods mentioned online have worked for
* multithread env).
*/
int SqliteDatabaseAccess::initDatabase()
{
    int i, rc;
    
    rc = sqlite3_open("csv.db", &db);
    if(rc != SQLITE_OK){
        Log().error(__func__,"DB open failed");
        return 1;
    }
    return 0;
}

SqliteDatabaseAccess::~SqliteDatabaseAccess()
{
    int rc;
    rc = sqlite3_close(db);
    if(rc != SQLITE_OK){
        Log().error(__func__,"DB open failed");
    }
}

static int rowCallback(void *data, int argc, char **argv, char **azColName)
{
   int i;
   std::string *mdata = (static_cast<std::string*>(data));
   
   for(i = 0; i < argc - 1; i++){
      mdata[i] =  argv[i];
   }

   return 0;
}

/* getRowValues(): Method returns the row values as a string array for the specified row index.
* return empty pointer string if the row index has overflowed/undeflowed or any error.
*/
std::string* SqliteDatabaseAccess::getRowValues(TableData* tData, int rowNum)
{
    std::string rowCmd = "SELECT * FROM " + tData->tableID + " where id=" 
                        + std::to_string(rowNum) + ";";
    std::string *feilds = new std::string[tData->metadata->columns];
    char* sqlErrMsg = NULL;
    int rc;
    
    rc = sqlite3_exec(db, rowCmd.c_str(), rowCallback, feilds, &sqlErrMsg);
    if(rc != SQLITE_OK){
        Log().error(__func__, "failed to fetch values");
        //DEBUG_ERR(__func__, *sqlErrMsg);
        if(sqlErrMsg)
            sqlite3_free(sqlErrMsg);
        return NULL;
    }

    return feilds;
}

static int colCallback(void *data, int argc, char **argv, char **azColName)
{
   int i;
   std::list<std::string> *mdata = (static_cast<std::list<std::string>*>(data));
   
   for(i = 0; i < argc; i++){
      if(argv[i])
         mdata->push_back(argv[i]);
   }

   return 0;
}

/* getColumnValues(): This method returns the array of string values for a particular column with specfied depth for 
* that table.
* This method return the list os values for a particular column identified by name of column and how deep should the
* values be fetched from db. Returns null string pointer in case of any errors. 
*/
std::string* SqliteDatabaseAccess::getColumnValues(std::string tableName, std::string columnName, int rows)
{
    std::string colCmd = "SELECT " + columnName + " FROM " + tableName + ";"; 
    std::string *feilds = new std::string[rows];
    std::list<std::string> data;
    char* sqlErrMsg = NULL;
    int rc, j = 0;
    
    rc = sqlite3_exec(db, colCmd.c_str(),colCallback , &data, &sqlErrMsg);
    if(rc != SQLITE_OK){
        sqlite3_free(sqlErrMsg);
        return NULL;
    }
    
    for(auto i = data.begin(); i != data.end(); i++, j++){
        if(j == rows) break;
        feilds[j] = *i;
    }
    return feilds;
}

/* writeValue(): This method executes the sql query passed as argument. In case of error returns error code of splite3. 
*/
int SqliteDatabaseAccess::writeValue(const char * sqlQuery)
{
    char* sqlErrMsg = NULL;
    int rc = 0;
    
    rc = sqlite3_exec(db, sqlQuery,NULL , 0, &sqlErrMsg);
    if(rc != SQLITE_OK){
        Log().error(__func__, sqlQuery," :db write command failed reason:" );
        sqlite3_free(sqlErrMsg);
    }

    return rc;
}

static int read_callback(void *data, int argc, char **argv, char **azColName) 
{
    std::string *mdata = (static_cast<std::string*>(data));
    int i;

    if(colNum < 0){
        for(i = 0; i < argc; i++){
            *mdata += argv[i];
            if(i < argc - 1)
                *mdata +=  ",";
        }
        *mdata += ";";
    } else if(colNum >= argc)
        mdata = NULL;
    else
        *mdata = argv[colNum];
    return 0;
}

/* readValue(): This method returns the result of sqlquery passed with the column index.
* This method is used for reading values of a particular row in a column. Returns null string pointer in case of error.
*/
std::string* SqliteDatabaseAccess::readValue(const char* sqlQuery, int column)
{
    char* sqlErrMsg = NULL;
    int rc = 0;
    std::string *str = new std::string;

    colNum = column;
    rc = sqlite3_exec(db, sqlQuery,read_callback , (void*)str, &sqlErrMsg);
    if(rc != SQLITE_OK){
        Log().error(__func__, "db write command failed:");
        //DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
        return NULL;
    }

    return str;
}

static int column_head_callback(void *data, int argc, char **argv, 
                char **azColName){
    int i;
    std::string *mdata = (static_cast<std::string*>(data));

    for(i = 0; i < argc - 1; i++){
        mdata[i] = azColName[i];
    }
    return 0;
}

/* getColumnNames(): Returns the column names as argument for the given table with total columns to retrive.
*/
std::string* SqliteDatabaseAccess::getColumnNames(std::string tableID, int cols)
{
    char* sqlErrMsg = NULL;
    int rc = 0;
    std::string *colNames = new std::string[cols];
    std::string sqlQuery = "SELECT * FROM " + tableID + " WHERE ID=1;";

    rc = sqlite3_exec(db, sqlQuery.c_str(), column_head_callback, colNames, NULL);
    if(rc != SQLITE_OK){
        Log().error(__func__, "db get command failed:");
        //DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
        return NULL;
    }

    return colNames;
}