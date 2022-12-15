#ifndef SOCKET_H
#define SOCKET_H
#include <iostream>
#include "../thread_pool.hpp"

extern pthread_cond_t socket_cond;
extern std::string computeID;

struct socket{
    struct thread_pool *thread;
    std::string port;
    std::string hostname;
    bool socketShouldStop;
};

struct socket* init_socket(struct thread_pool *, std::string *);

#endif