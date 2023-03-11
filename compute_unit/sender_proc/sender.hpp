#ifndef FORWD_H_
#define FORWD_H_
#include "forward_stack.hpp"
#include "../lib/nlohmann/json-schema.hpp"

using json = nlohmann::json;

int send_packet(std::string, std::string, int, TaskPriority);
json getPacket(void);
#endif