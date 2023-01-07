#ifndef ALGO_H
#define ALGO_H
#include <sqlite3.h>
#include "cand_elim.hpp"
#include "example.hpp"
//bound to change once build root is made
#define TOT_ALGO 2

int sched_algo(struct thread_pool*, struct table*);
void dealloc_table_dat(struct table *tData);

//add function prototypes below this
struct process* init_example_algo(struct table*);
struct process* init_ce_algorithm(struct table*);

static struct process* (*avial_algo[TOT_ALGO])(struct table*) = {
    init_example_algo,
    init_ce_algorithm
};

typedef Base* BaseMaker();

template <class X> Base* make() {
  return new X;
}

static BaseMaker* makers[TOT_ALGO] = { 
    make<example>, 
    make<candidateElimination> 
};
#endif