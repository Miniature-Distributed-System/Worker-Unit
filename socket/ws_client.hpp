#ifndef WS_CLIENT_H
#define WS_CLIENT_H
#include<string>
#include "../lib/nlohmann/json-schema.hpp"

using json = nlohmann::json;
extern std::unique_ptr<nlohmann::json> ws_client_launch(std::unique_ptr<nlohmann::json> packet);

#endif
