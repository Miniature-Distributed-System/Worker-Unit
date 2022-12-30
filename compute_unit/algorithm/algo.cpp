#include <stdlib.h>
#include "algo.hpp"
#include "../data_processing.hpp"
#include "../receiver_proc/debug_rp.hpp"


int sched_algo(struct table *tData)
{
    short algoIndex;

    if(!tData){
        DEBUG_ERR(__func__, "table data is uninitlised!");
        return EXIT_FAILURE;
    }

    algoIndex = tData->algorithmType;
    DEBUG_MSG(__func__, "Sched algorihm type:",algoIndex);

    return EXIT_SUCCESS;
}

static int rowCallback(void *data, int argc, char **argv, char **azColName)
{
   int i;
   std::string *mdata = (static_cast<std::string*>(data));
   
   for(i = 1; i < argc; i++){
      mdata[i] =  argv[i];
   }

   return 0;
}

std::string* getRow(int rowNum, std::string tableID, unsigned int rows){
    std::string rowCmd = "SELECT * FROM " + tableID;
    std::string row;
    std::string *feilds = new std::string[rows];
    char* sqlErrMsg;
    int rc;

    rc = sqlite3_exec(db, rowCmd.c_str(),rowCallback , &row, &sqlErrMsg);
    if(rc != SQLITE_OK){
            DEBUG_ERR(__func__, *sqlErrMsg);
            sqlite3_free(sqlErrMsg);
            return NULL;
    }
    
    return feilds;
}
