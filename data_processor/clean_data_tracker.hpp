#ifndef CLN_DATA_TRACKER_H
#define CLN_DATA_TRACKER_H
#include <map>
#include <string>
#include "../include/process.hpp"

void schedule_clean_phase(std::string tableName);
int update_clean_stages(TableData *tableData);
extern std::map<TableData *, int> cleanStageMap;

#endif