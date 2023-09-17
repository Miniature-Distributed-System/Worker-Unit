#ifndef PACKET_H
#define PACKET_H

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

enum SenderDataType {
    RECV_ERR = 0,   // Receiver error
    RESET_TIMER,    // Receiver is still processing the user data
    DAT_ERR,        // Receiver encountered error parsing json data
    DAT_RECVD,      // Received csv data
    INTR_SEND,      // Sending intermidiate data
    FRES_SEND,      // Sending result data
    SEIZE,          // Sender is seizing accepting any new csv for processing
    DAT_NULL = 100,
};

enum server_packet_status {
    SP_HANDSHAKE    = 1 << 0,   //Server handshake packet
    SP_DATA_SENT    = 1 << 1,   //Server sent csv data
    SP_INTR_ACK     = 1 << 2,   //Server received the intermediate data
    SP_FRES_ACK     = 1 << 3,   //Server received the result(includes res tableID)
    SP_ERR          = 1 << 4    //Server received packet was corrupt
};

#endif