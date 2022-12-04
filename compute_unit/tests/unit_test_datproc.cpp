#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <iostream> 
#include "../data_processing.hpp"
#include "../algorithm/algo.hpp"
#include "../thread_pool.hpp"
#include "../receiver_proc/debug_rp.hpp"

struct table *tData;

void debug_data_proc_phase(int expected_cols, int expected_rows){
   if(tData->metadata->cols == expected_cols && 
            tData->metadata->rows == expected_rows){
      DEBUG_MSG(__func__, "Metadata inited properly");
   }
   else {
      if(tData->metadata->cols != expected_cols)
         DEBUG_ERR(__func__, "Expected cols:", expected_cols,
                  " Observed cols:", tData->metadata->cols);
      else
         DEBUG_ERR(__func__, "Expected rows:", expected_rows, 
                  " Observed rows:", tData->metadata->rows);
   }
}

void debug_thread_pool(){
   DEBUG_MSG(__func__,"Number of process in table:", threadPoolCount);
}

int init_test(){
   int rc = 0;
   rc = register_algo();
   rc = init_thread_pool();
   return rc;
}
int main(int argc, char* argv[]) {
   int rc;
   int expected_rows = 3, expected_cols = 5;
   tData = new table;
   

   DEBUG_MSG(__func__, "Starting dataprocessing unit test...\n");

   rc = init_test();
   if(rc == EXIT_FAILURE){
      DEBUG_ERR(__func__, "initilization failed");
      return 0;
   }

   DEBUG_MSG(__func__, "Available algorithms:", TOT_ALGO);
   tData->tableID = "myTable"; //data-base-schema: sky|airtemp|humidity|wind|ID
   tData->algorithmType = 1;
   tData->priority = 1;
   //this needs refactor bad practise
   tData->firstColName = "sky";
   processData(tData);

   debug_data_proc_phase(expected_cols, expected_rows);
   debug_thread_pool();
   exit_thread_pool();
   DEBUG_MSG(__func__, "dataprocessing unit test ended");
   return 0;
}