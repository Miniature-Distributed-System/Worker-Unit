#ifndef RP_DEBUG_H
#define RP_DEBUG_H
#include <string>
#include "debug_rp.impl"

template<typename... Args>
void DEBUG_MSG(std::string fun_name, Args... args);

template<typename... Args>
void DEBUG_ERR(std::string fun_name, Args... args);

#endif