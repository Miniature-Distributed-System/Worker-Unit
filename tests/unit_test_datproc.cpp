#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <iostream>
#include "../sql_access.hpp"
#include "../data_processor.hpp"
#include "../algorithm/algo.hpp"
#include "../scheduler/thread_pool.hpp"
#include "../include/debug_rp.hpp"
#include "../include/task.hpp"

int sched_task(struct thread_pool *threadPoolHead, struct process *newProc, 
                void *args, int prior)
{
   int ret = JOB_PENDING;
   while(ret == JOB_PENDING)
      ret = newProc->start_proc(args);
   newProc->end_proc(args);
   return 0;
}

//Dummy methods
int send_packet(std::string data, std::string tableID, int statusCode)
{
    return 0;
}

int sched_algo(struct thread_pool *thread, struct table *tData)
{
   return 0;
}


int main(int argc, char* argv[])
{
   struct data_proc_container* container = new data_proc_container;
   struct table *tData = new table;
   struct thread_pool *thread;
   int rc;
   
   init_db();
   container->cols = 7;
   container->rows = 12;
   container->colHeaders = new std::string[7]{"sky", "airtemp", "humidity", "wind", "water", "forecast", "enjoysport"};
   tData->tableID = "mytable1";
   tData->priority = 1;
   tData->algorithmType = 1;
   container->tData = tData;

   init_data_processor(thread, container);
   
   DEBUG_MSG(__func__, "dataprocessing unit test ended");
   return 0;
}