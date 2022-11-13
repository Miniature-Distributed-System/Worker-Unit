#ifndef PACKET_H
#define PACKET_H
#include "json.hpp"

#define P_RESET         1 << 0 //Reset packet
#define P_HANDSHAKE     1 << 1 //Inital handshake
#define P_RSEND_DATA    1 << 2 //Resend data due to failure
#define P_RECV_DATA     1 << 3 //Data received correctly
#define P_SEIZE         1 << 4 //Don't send any more packets as
                               //compute-unit encountered fatal error

using json = nlohmann::json;
extern json packet;

#endif