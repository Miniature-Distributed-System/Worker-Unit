#ifndef RECV_PORT_H_
#define RECV_PORT_H_
#include <iostream>
#include "../sender_proc/sender.hpp"
#include "../scheduler/thread_pool.hpp"

int init_receiver(struct ThreadPool*, json);
#endif