#include "sql_access.hpp"
#include "include/debug_rp.hpp"
#include "data_processor.hpp"

sqlite3 *db;
sem_t db_lock;

sqlite3* init_db(void)
{
    int i, rc;
    sem_init(&db_lock, 0, 1);
    //needs to be nuked make that var global scope
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
                        + std::to_string(rowNum);
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
    char* sqlErrMsg;
    int rc = 0;
    
    sem_wait(&db_lock);
    rc = sqlite3_exec(db, sqlQuery,NULL , 0, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db write command failed");
        DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
    }
    sem_post(&db_lock);

    return rc;
}

static int read_callback(void *data, int argc, char **argv, char **azColName) 
{
    int i;
    int *metdata = (int*)data;

    *metdata = atoi(argv[0]);
   
    return 0;
}

int sql_read_value(const char* sqlQuery, int* value)
{
    char* sqlErrMsg;
    int rc = 0;

    rc = sqlite3_exec(db, sqlQuery,read_callback , (void*)value, &sqlErrMsg);
    if(rc != SQLITE_OK){
        DEBUG_ERR(__func__, "db write command failed");
        DEBUG_ERR(__func__, sqlQuery);
        sqlite3_free(sqlErrMsg);
        return EXIT_FAILURE;
    }
    return 0;
}