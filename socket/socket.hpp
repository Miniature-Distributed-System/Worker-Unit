#ifndef SOCKET_H
#define SOCKET_H
#include <iostream>
#include "../scheduler/thread_pool.hpp"
#include "../include/flag.h"

extern pthread_cond_t socket_cond;
extern std::string computeID;
extern pthread_t wsClientThread;
extern Flag quickSendMode;
extern Flag seizeMode;
extern Flag initSender;

struct socket{
    struct ThreadPool *thread;
    std::string port;
    std::string hostname;
    bool socketShouldStop;
};

struct socket* init_socket(struct ThreadPool *, std::string *);
void exit_socket(struct socket *soc);

#endif