#ifndef ALGO_H
#define ALGO_H
#include <sqlite3.h>
//bound to change once build root is made
#define TOT_ALGO 2

extern sqlite3 *db;
struct algo_proc{
    int (*start_proc)(void*);
    int (*pause_proc)(void*);
    int (*end_proc)(void*);
};

struct algo_data{
    struct algo_proc *algo_proc_ptrs[TOT_ALGO];
    short int inited_algo;
};

extern struct algo_data *algoData;
int init_algo(struct algo_proc*);
int register_algo();
int sched_algo(struct table*);

//add function prototypes below this
int init_example_algo();
int init_example_algo2();

extern int (*avial_algo[TOT_ALGO])();

#endif