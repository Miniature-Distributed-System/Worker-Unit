#ifndef WS_CLIENT_H
#define WS_CLIENT_H
#include<string>
#include "../lib/nlohmann/json-schema.hpp"

using json = nlohmann::json;

extern json ws_client_launch(struct socket*, json);

#endif