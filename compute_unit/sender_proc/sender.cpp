#include "../data_processing.hpp"
#include "../socket/packet.hpp"
#include "../include/debug_rp.hpp"
#include "../include/debug_rp.hpp"
#include "../socket/socket.hpp"
#include "forward_stack.hpp"

int send_packet(std::string data, std::string tableID, int statusCode)
{
    push_forward_stk(data, tableID, statusCode);
}

json create_packet(json packet, struct fwd_stack* item)
{
    std::string body;
    switch(item->statusCode)
    {
        case PROC_ERR: //data not received correctly
            packet["head"] = P_RSEND_DATA;
            if(!item->data.empty())
                body = item->data;
            //aggrigate all failed tableIDs
            while(show_front()->statusCode == -1){
                item = pop_forward_stk();
                if(!item->data.empty())
                    body += "," + item->data;
            }
            packet["id"] = computeID.c_str();
            packet["body"]["id"] = body.c_str();
            break;
        case RECV_ERR: //packet was corrupt on arrival
            packet["head"] = P_RSEND_DATA;
            packet["id"] = computeID.c_str();
            break;
        case P_RECV_DATA: //all data received correctly
            packet["head"] = P_RECV_DATA;
            packet["id"] = computeID.c_str();
            packet["body"]["id"] = item->tableID.c_str();
            if(!item->data.empty()){
                packet["head"] = P_RECV_DATA & P_DATSENT;
                packet["body"]["data"] = item->data.c_str();
            }
            break;
        case RES_SEND: //result data is being sent over by next fwd packet
            packet["head"] = P_DATSENT;
            packet["id"] = computeID.c_str();
            packet["body"]["id"] = item->tableID.c_str(); 
            packet["body"]["data"] = item->data.c_str();
            break;
        default:
            packet["head"] = P_RESET;
            packet["id"] = computeID.c_str();
    }
    return packet;
}

json getPacket(void)
{
    json packet;
    struct fwd_stack* item;
    std::string body;
    item = pop_forward_stk();

    //If there are no items it must be the first time we are starting out
    if(!item && computeID.empty())
    {
        DEBUG_MSG(__func__, "initial handshake packet");
        packet["head"] = P_HANDSHAKE;
    } else {
        packet = create_packet(packet, item);
    }
    DEBUG_MSG(__func__, "sender packet ready");
    return packet;
}
