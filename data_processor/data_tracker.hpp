#ifndef _CLN_STAGE_TRACKER_H
#define _CLN_STAGE_TRACKER_H
#include <map>
#include <string>
#include "../include/process.hpp"
#include "instance_data.hpp"

void schedule_validate_phase(TableData *tableData, InstanceData *instanceData);
int update_clean_stages(TableData *tableData);
extern std::map<TableData *, int> cleanStageMap;

#endif