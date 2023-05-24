#include <string>
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "algorithm_scheduler.hpp"

JobStatus example_start_process(void *data){
    Log().info(__func__, "Example start process");
    return JOB_DONE;
}

JobStatus example_pause_process(void *data){
    Log().info(__func__, "Example pause process");
    return JOB_DONE;
}

void example_end_process(void *data){
    Log().info(__func__, "Example end process");
}

struct ProcessStates* example_proc = new ProcessStates{
    .start_proc = example_start_process,
    .pause_proc = example_pause_process,
    .end_proc = example_end_process
};

AlgorithmExportPackage init_example_algo(std::string tableId, std::string instanceId, int start, int end, int columns)
{
    return AlgorithmExportPackage(example_proc, NULL);
}

AlgorithmExportPackage init_example_finalize(std::vector<void *>, std::string, TaskPriority)
{
    return AlgorithmExportPackage(example_proc, NULL);
}

