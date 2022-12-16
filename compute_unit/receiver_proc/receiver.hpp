#ifndef RECV_PORT_H_
#define RECV_PORT_H_
#include <iostream>
#include <sqlite3.h> 
#include "../socket/json.hpp"
#include "../thread_pool.hpp"
#include "../data_processing.hpp"
#define ERR_DBOPEN -1

using json = nlohmann::json;

struct receiver {
    struct thread_pool *thread;
    sqlite3 *db;
    json packet;
    int packetStatus;
    int receiverStatus;
    struct table *table;
};

int init_receiver(struct thread_pool*, json);
#endif