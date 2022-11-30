#include <string>
#include "algo.hpp"
#include "../receiver_proc/debug_rp.hpp"

int example_start_process(void *data){
    DEBUG_MSG(__func__, "Example start process");
}

int example_pause_process(void *data){
    DEBUG_MSG(__func__, "Example pause process");
}

int example_end_process(void *data){
    DEBUG_MSG(__func__, "Example end process");
}

struct algo_proc example_proc = {
    .start_proc = example_start_process,
    .pause_proc = example_pause_process,
    .end_proc = example_end_process
};
int init_example_algo(){
    init_algo(&example_proc);
    return 0;
}