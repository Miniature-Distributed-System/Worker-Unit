#ifndef PACKET_H
#define PACKET_H

#include "json.hpp"
using json = nlohmann::json;

#define P_RESET         1 << 0 
#define P_HANDSHAKE     1 << 1 //Inital handshake
#define P_RSEND_DATA    1 << 2 //Resend data due to failure
#define P_RECV_DATA     1 << 3 //Data received correctly
#define P_DATSENT       1 << 4 //Data sent to server
#define P_SEIZE         1 << 5 //Don't send any more packets CU at limit

enum packet_code {
    NO_ERR = 0,
    PROC_ERR,
    RECV_ERR,
    RES_SEND,
};

#endif