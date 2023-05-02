#include <list>
#include "sql_access.hpp"
#include "include/debug_rp.hpp"
#include "data_processor.hpp"

static int colNum;

int DatabaseAccess::initDatabase()
{
    int i, rc;
    sem_init(&dataBaseLock, 0, 1);
    
    rc = sqlite3_open("csv.db", &db);
    if(rc){
        DEBUG_ERR(__func__,"DB open failed");
        return 1;
    }
    return 0;
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

std::string* DatabaseAccess::getRowValues(struct table* tData, int rowNum)
{
    std::string rowCmd = "SELECT * FROM " + tData->tableID + " where id=" 
                        + std::to_string(rowNum) + ";";
    std::string *feilds = new std::string[tData->metadata->cols];
    char* sqlErrMsg;
    int rc;
    
    sem_wait(&dataBaseLock);
    rc = sqlite3_exec(db, rowCmd.c_str(),rowCallback , feilds, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, *sqlErrMsg);
        sqlite3_free(sqlErrMsg);
        sem_post(&dataBaseLock);
        return NULL;
    }

    sem_post(&dataBaseLock);
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

std::string* DatabaseAccess::getColumnValues(std::string tableName, std::string columnName, int rows)
{
    std::string colCmd = "SELECT " + columnName + " FROM " + tableName + ";"; 
    std::string *feilds = new std::string[rows];
    std::list<std::string> data;
    char* sqlErrMsg;
    int rc, j = 0;
    
    sem_wait(&dataBaseLock);
    rc = sqlite3_exec(db, colCmd.c_str(),colCallback , &data, &sqlErrMsg);
    if(rc != SQLITE_OK){
        sqlite3_free(sqlErrMsg);
        sem_post(&dataBaseLock);
        return NULL;
    }
    for(auto i = data.begin(); i != data.end(); i++, j++)
      feilds[j] = *i;
    sem_post(&dataBaseLock);
    return feilds;
}

int DatabaseAccess::writeValue(const char * sqlQuery)
{
    char* sqlErrMsg = NULL;
    int rc = 0;
    
    sem_wait(&dataBaseLock);
    rc = sqlite3_exec(db, sqlQuery,NULL , 0, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db write command failed");
        DEBUG_ERR(__func__, sqlQuery," reason:" ,*sqlErrMsg);
        sqlite3_free(sqlErrMsg);
    }
    sem_post(&dataBaseLock);

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

std::string* DatabaseAccess::readValue(const char* sqlQuery, int column)
{
    char* sqlErrMsg;
    int rc = 0;
    std::string *str = new std::string;

    sem_wait(&dataBaseLock);
    colNum = column;
    rc = sqlite3_exec(db, sqlQuery,read_callback , (void*)str, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db write command failed");
        DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
        sem_post(&dataBaseLock);
        return NULL;
    }
    sem_post(&dataBaseLock);

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

std::string* DatabaseAccess::getColumnNames(std::string tableID, int cols)
{
    char* sqlErrMsg;
    int rc = 0;
    std::string *colNames = new std::string[cols];
    std::string sqlQuery = "SELECT * FROM " + tableID + " WHERE ID=1;";

    sem_post(&dataBaseLock);
    rc = sqlite3_exec(db, sqlQuery.c_str(), column_head_callback, colNames, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db get command failed");
        DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
        sem_post(&dataBaseLock);
        return NULL;
    }
    sem_post(&dataBaseLock);

    return colNames;
} 