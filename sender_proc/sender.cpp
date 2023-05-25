#include <unistd.h>
#include "../include/packet.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../socket/socket.hpp"
#include "../services/stats_engine.hpp"
#include "../configs.hpp"
#include "sender.hpp"

/* pushPacket(): sends the data passed as arguments to the forward port/ sender stack.
 * This queuded data is eventually converted into packet and sent out to the server. We can send various types of
 * packets refer packet.hpp for more details.
*/
int SenderSink::pushPacket(std::string data, std::string tableID, packet_code statusCode, TaskPriority priority)
{
    //We want to immidiatly tell server that node processing service is suspended
    if(statusCode == SEIZE)
        fwdStack.pushFrontForwardStack(data, tableID, statusCode, priority);
    else {
        fwdStack.pushToForwardStack(data, tableID, statusCode, priority);
        quickSendMode.setFlag();
    }
    return 0;
}

/* getPacketHead(): internal method for getting packet head status codes which shall be used for represeting the packet
 * type at the server and help diffrentiate between the data being sent to the server. out[uts value of type enum.
*/
compute_packet_status getPacketHead(packet_code status)
{
    switch(status)
    {
        case RECV_ERR:
        case DAT_ERR: //TO-DO: not yet deceided how to handle this case
            //packet was corrupt on arrival
            return P_ERR;
        case DAT_RECVD: 
            //all data received correctly
            return P_DATA_ACK;
        case INTR_SEND: 
            //intermediate result data is sent over by next fwd packet
            return P_INTR_RES;
        case FRES_SEND: 
            //final result data is being sent over by next fwd packet
            return P_FINAL_RES;
        case SEIZE:
            //let the server know there is a curfew in the compute node
            seizeMode.setFlag();
            return P_SEIZE;
        default:
            return P_RESET;
    }
}

/* create_packet(): internal method which is used to create packets from ForwardStackPackage() items.
 * it creates the packets structure with the appropriate body, head and id fields. it creates default packets if the
 * item passed is found to be null.
*/
json create_packet(struct ForwardStackPackage item)
{
    json packet;
    packet_code statusCode;

    statusCode = item.statusCode;

    //Here we onlt append head to the packet
    if(seizeMode.isFlagSet()){
         /* curfew isnt lifted unless seizeMode is explicitly unset by core processes. This may be set due to either 
         node being overloaded or the node shutdown sequence being singnalled by the user. */
        packet["head"] = getPacketHead(statusCode) | P_SEIZE;
    } else if(quickSendMode.isFlagSet()){
        // stack is not empty that means there is one more item behind this current item, so proceed to quicksend mode
        packet["head"] = getPacketHead(statusCode) | P_QSEND;
        if(senderSink.isForwardStackEmpty())
            quickSendMode.resetFlag();
    } else {
        packet["head"] = getPacketHead(statusCode);
    }

    packet["id"] = globalConfigs.getWorkerId().c_str();
    //Here we append rest of the fields to packet
    switch(statusCode)
    {
        case RECV_ERR:
        case DAT_ERR:
            packet["body"]["id"] = item.tableID.c_str();
            packet["body"]["data"] = item.data.c_str();
            break;
        case RESET_TIMER:
        case DAT_RECVD:
            packet["body"]["id"] = item.tableID.c_str();
            packet["body"]["priority"] = item.priority;
            break;
        case INTR_SEND:
        case FRES_SEND:
            packet["body"]["id"] = item.tableID.c_str();
            packet["body"]["data"] = item.data.c_str();
            //Only user generated data has priority
            packet["body"]["priority"] = item.priority;
            break;
        default:
            //this should exit from spitfire mode and slow down the up/down data send rate between server and node.
            quickSendMode.resetFlag();
    }
    
    packet["stats"] = statsEngine.toJson();

    //DEBUG_MSG(__func__, "packet created body: ", packet.dump(), " status code: ", statusCode);

    return packet;
}

int SenderSink::matchItemInAwaitStack(int statusCode, std::string tableID)
{
    return fwdStack.awaitStack.matchItemWithAwaitStack(statusCode, tableID);
}

/* getPacket(): This is used to get the next packet to the socket.
* It calls various methods to create a packet with appropriate fields using the ForwardStack which have the queued
* items that are to be sent to the server. sends default packet if the workerId variable is empty, this is done 
* because without a an allocated name we cant procceed further. We need a name to identify ourself and tell server
* that a certain packet came from this node and the packet needs to be acknowledged to that node only.
*/
json SenderSink::popPacket(void)
{
    json packet;
    std::string body;

    //If there are no items it must be the first time we are starting out
    if(globalConfigs.getWorkerId().empty())
    {
        if(initSender.isFlagSet()){
            Log().info(__func__,"worker ID not set yet");
            while(globalConfigs.getWorkerId().empty());
            if(!globalConfigs.getWorkerId().empty()){
                initSender.resetFlag();
                packet = create_packet(fwdStack.popForwardStack());
            }
        } else {
            Log().info(__func__, "initial handshake packet");
            packet["head"] = P_HANDSHAKE;
            packet["id"] = "";
            initSender.setFlag();
        }
    } else {
        //Get packet to be sent to the server
        packet = create_packet(fwdStack.popForwardStack());
    }
    Log().info(__func__, "sender packet ready");

    return packet;
}
