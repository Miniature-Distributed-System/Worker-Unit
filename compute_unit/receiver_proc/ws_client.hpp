#ifndef WS_CLIENT_H
#define WS_CLIENT_H
#include<string>
#include "packet.hpp"

using json = nlohmann::json;

extern json ws_client_launch(std::string, std::string);

#endif