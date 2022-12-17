#ifndef FORWD_H_
#define FORWD_H_
#include "../socket/json.hpp"

using json = nlohmann::json;

int send_packet(std::string, std::string, int);
json getPacket(void);
#endif