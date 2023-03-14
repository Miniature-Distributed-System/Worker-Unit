#ifndef FORWD_H_
#define FORWD_H_
#include "forward_stack.hpp"
#include "../lib/nlohmann/json-schema.hpp"

using json = nlohmann::json;

int send_packet(std::string data, std::string tableID, packet_code statusCode, TaskPriority priority);
json getPacket(void);
#endif