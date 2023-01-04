#ifndef RECV_PORT_H_
#define RECV_PORT_H_
#include <iostream>
#include "../socket/json.hpp"
#include "../thread_pool.hpp"
#include "../data_processing.hpp"
#define ERR_DBOPEN -1

using json = nlohmann::json;

struct receiver {
    struct thread_pool *thread;
    json packet;
    int packetStatus;
    int receiverStatus;
    std::string tableID;
    struct data_proc_container* container;
};

int init_receiver(struct thread_pool*, json);
#endif