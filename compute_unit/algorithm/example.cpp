#include <string>
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"

int example_start_process(void *data){
    DEBUG_MSG(__func__, "Example start process");
    return 0;
}

int example_pause_process(void *data){
    DEBUG_MSG(__func__, "Example pause process");
    return 0;
}

int example_end_process(void *data){
    DEBUG_MSG(__func__, "Example end process");
    return 0;
}

struct process* example_proc = new process{
    .start_proc = example_start_process,
    .pause_proc = example_pause_process,
    .end_proc = example_end_process
};
struct process* init_example_algo(struct table* tab)
{
    return example_proc;
}