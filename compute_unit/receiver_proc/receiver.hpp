#ifndef RECV_PORT_H_
#define RECV_PORT_H_
#include <iostream>
#include "../sender_proc/sender.hpp"
#include "../thread_pool.hpp"

int init_receiver(struct thread_pool*, json);
#endif