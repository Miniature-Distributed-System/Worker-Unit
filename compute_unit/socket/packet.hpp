#ifndef PACKET_H
#define PACKET_H

#include "json.hpp"
using json = nlohmann::json;

enum compute_packet_status {
    P_RESET     = 1 << 0, //Empty packet essentially
    P_HANDSHAKE = 1 << 1, //Handshake snd,ack packet
    P_DATA_ACK  = 1 << 2, //Table received ack
    P_INTR_RES  = 1 << 3, //Intermediate result sent
    P_FINAL_RES = 1 << 4, //Final result inside packet
    P_ERR       = 1 << 5, //Packet error
    P_QSEND     = 1 << 6, //Client is in quick send mode
    P_SEIZE     = 1 << 7, //Client has seized receiving packets
};

enum packet_code {
    NO_ERR = 0,
    PROC_ERR,
    RECV_ERR,
    RES_SEND,
};

#endif