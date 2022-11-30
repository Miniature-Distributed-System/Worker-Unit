#include <string>
#include "algo.hpp"
#include "../receiver_proc/debug_rp.hpp"

int example2_start_process(void *data){
    DEBUG_MSG(__func__, "Example start process");
}

int example2_pause_process(void *data){
    DEBUG_MSG(__func__, "Example pause process");
}

int example2_end_process(void *data){
    DEBUG_MSG(__func__, "Example end process");
}

struct algo_proc example_proc2 = {
    .start_proc = example2_start_process,
    .pause_proc = example2_pause_process,
    .end_proc = example2_end_process
};
int init_example_algo2(){
    init_algo(&example_proc2);
    return 0;
}