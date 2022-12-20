#include <stdlib.h>
#include "algo.hpp"
#include "../data_processing.hpp"
#include "../receiver_proc/debug_rp.hpp"

struct algo_data *algoData;
int (*avial_algo[TOT_ALGO])() = {
    init_example_algo,
    init_example_algo2
};

int register_algo(void)
{
    algoData = (struct algo_data*)calloc(1, sizeof(struct algo_data));
    int i, rc;
    
    for(i = 0; i < TOT_ALGO; i++){
        rc = avial_algo[i]();
        if(rc == EXIT_FAILURE){
            DEBUG_ERR(__func__,"Failed to init algorithm_idx:",i);
        }
    }

    return EXIT_SUCCESS;
}

int init_algo(struct algo_proc* aptr)
{    
    if(aptr == NULL){
        return EXIT_FAILURE;
    }
    if(aptr->start_proc == NULL || aptr->pause_proc == NULL 
                || aptr->end_proc == NULL){
                    return EXIT_FAILURE;
    }
    algoData->algo_proc_ptrs[algoData->inited_algo++] = aptr;

    return EXIT_SUCCESS;
}

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
