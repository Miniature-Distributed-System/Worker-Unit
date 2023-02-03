#include <cassert>
#include "sql_access.hpp"
#include "include/debug_rp.hpp"
#include "data_processor.hpp"

sqlite3 *db;
sqlite3_mutex *db_mutex;
static int colNum;

sqlite3* init_db(void)
{
    int i, rc;
    //sem_init(&db_lock, 0, 1);
    //needs to be nuked make that var global scope
    assert(sqlite3_config(SQLITE_CONFIG_MULTITHREAD) == SQLITE_OK);
    db_mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
    
    rc = sqlite3_open("csv.db", &db);
    if(rc){
        DEBUG_ERR(__func__,"DB open failed");
        return NULL;
    }
    
    return db;
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

std::string* get_row(struct table* tData, int rowNum)
{
    std::string rowCmd = "SELECT * FROM " + tData->tableID + " where id=" 
                        + std::to_string(rowNum) + ";";
    std::string *feilds = new std::string[tData->metadata->cols];
    char* sqlErrMsg;
    int rc;
    
    rc = sqlite3_exec(db, rowCmd.c_str(),rowCallback , feilds, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, *sqlErrMsg);
        sqlite3_free(sqlErrMsg);
        return NULL;
    }

    return feilds;
}

int sql_write(const char * sqlQuery)
{
    char* sqlErrMsg = NULL;
    int rc = 0;
    
    sqlite3_mutex_enter(db_mutex);
    rc = sqlite3_exec(db, sqlQuery,NULL , 0, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db write command failed");
        DEBUG_ERR(__func__, sqlQuery," reason:" ,*sqlErrMsg);
        sqlite3_free(sqlErrMsg);
    }
    sqlite3_mutex_leave(db_mutex);

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

std::string* sql_read(const char* sqlQuery, int column)
{
    char* sqlErrMsg;
    int rc = 0;
    colNum = column;
    std::string *str = new std::string;

    rc = sqlite3_exec(db, sqlQuery,read_callback , (void*)str, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db write command failed");
        DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
        return NULL;
    }

    return str;
}

static int column_head_callback(void *data, int argc, char **argv, 
                char **azColName){
   int i;
   std::string *mdata = (static_cast<std::string*>(data));

   for(i = 1; i<argc; i++){
      mdata[i] = azColName[i];
   }
   return 0;
}

std::string* sql_get_column_names(std::string tableID, int cols)
{
    char* sqlErrMsg;
    int rc = 0;
    std::string *colNames = new std::string[cols];
    std::string sqlQuery = "select * from " + tableID + " where ID=1;";

    rc = sqlite3_exec(db, sqlQuery.c_str(),column_head_callback , 
                    (void*)colNames, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db get command failed");
        DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
        return NULL;
    }
    return colNames;
}
