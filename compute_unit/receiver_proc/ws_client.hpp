#ifndef WS_CLIENT_H
#define WS_CLIENT_H
#include<string>

std::string data_buffer;
std::string packet;
int ws_client_launch(int, std::string);

#endif