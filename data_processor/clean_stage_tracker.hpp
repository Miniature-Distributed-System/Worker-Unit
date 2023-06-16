#ifndef _CLN_STAGE_TRACKER_H
#define _CLN_STAGE_TRACKER_H
#include <map>
#include <string>
#include "../include/process.hpp"

void schedule_clean_phase(TableData *tableData, Instance *instance);
int update_clean_stages(TableData *tableData);
extern std::map<TableData *, int> cleanStageMap;

#endif