#ifndef SOCKET_H
#define SOCKET_H
#include <iostream>
#include "../include/flag.h"

extern pthread_cond_t socket_cond;
extern pthread_t wsClientThread;
extern Flag quickSendMode;
extern Flag seizeMode;

struct socket{
    std::string port;
    std::string hostname;
    bool socketShouldStop;
};

struct socket* init_socket();
void exit_socket(struct socket *soc);

#endif