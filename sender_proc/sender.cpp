#include <unistd.h>
#include "../include/packet.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../socket/socket.hpp"
#include "../services/stats_engine.hpp"
#include "../configs.hpp"
#include "sender.hpp"

using nlohmann::json_schema::json_validator;

static json senderSchema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "Data Body of packet",
    "properties": {
      "head": {
          "description": "Packet identifier/info",
          "type": "number"
      },
      "id": {
          "description": "Worker identifier",
          "type": "string"
      }
    },
    "required": [
                 "head",
                 "id"
                 ],
    "type": "object"
}
)"_json;

/* pushPacket(): sends the data passed as arguments to the forward port/ sender stack.
 * This queuded data is eventually converted into packet and sent out to the server. We can send various types of
 * packets refer packet.hpp for more details.
*/
int SenderSink::pushPacket(std::string data, std::string tableID, SenderDataType statusCode, TaskPriority priority)
{
    //We want to immidiatly tell server that node processing service is suspended
    if(statusCode == SEIZE)
        fwdStack.pushFront(data, tableID, statusCode, priority);
    else {
        fwdStack.push(data, tableID, statusCode, priority);
        //quickSendMode.setFlag();
        // TO-DO: Needs refactoring and this needs re-thinking
        //globalSocket.setFlag(SOC_SETQS);
    }
    Log().info(__func__, "pushed table id to be sent:", tableID);
    return 0;
}

bool SenderSink::isValidPacket(json packet)
{
    json_validator validator;
    validator.set_root_schema(senderSchema);

    try{
        auto defaultPatch = validator.validate(packet);
        return true;
    }catch(const std::exception ex){
        return false;
    }
}

/* getPacketHead(): internal method for fetching header status codes to identify packet type at the server. Output is 
 * of type enum(check SenderDataType data structure for more information).
*/
compute_packet_status getPacketHead(SenderDataType status)
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
            globalSocket.setFlag(SOC_SEIZE);
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
    SenderDataType statusCode;

    statusCode = item.statusCode;

    //Here we onlt append head to the packet
    int socketStatus = globalSocket.getSocketStatus();
    if(socketStatus & SOC_SEIZE){
         /* curfew isnt lifted unless seizeMode is explicitly unset by core processes. This may be set due to either 
         node being overloaded or the node shutdown sequence being singnalled by the user. */
        packet["head"] = getPacketHead(statusCode) | P_SEIZE;
    } else if(!senderSink.isEmpty()){
        // stack is not empty that means there is one more item behind this current item, so proceed to quicksend mode
        packet["head"] = getPacketHead(statusCode) | P_QSEND;
        // if(senderSink.isEmpty())
        //     globalSocket.setFlag(SOC_SETQS);
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
            Log().debug(__func__, "data error packet created");
            break;
        case RESET_TIMER:
        case DAT_RECVD:
            packet["body"]["id"] = item.tableID.c_str();
            packet["body"]["priority"] = item.priority;
            Log().debug(__func__, "data received/timeout ack created");
            break;
        case INTR_SEND:
        case FRES_SEND:
            packet["body"]["id"] = item.tableID.c_str();
            packet["body"]["data"] = item.data.c_str();
            //Only user generated data has priority
            packet["body"]["priority"] = item.priority;
            Log().debug(__func__, "final/intermediate res packet created");
            break;
        default:
            //this should exit from quicksend mode and slow down the up/down data send rate between server and node.
            globalSocket.setFlag(SOC_NORMAL_MODE);
    }
    
    packet["stats"] = statsEngine.toJson();

    return packet;
}

int SenderSink::matchItemInAwaitStack(int statusCode, std::string tableID)
{
    return fwdStack.awaitStack.matchItem(statusCode, tableID);
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
        /* If handshake is already done don't resend it again as it will cause server to think of duplicate handshakes
           as different workers units and registering them */
        if(handshakeSent.isFlagSet()){
            Log().info(__func__,"Waiting to be assigned an ID by server...");
            while(globalConfigs.getWorkerId().empty());
            if(!globalConfigs.getWorkerId().empty()){
                Log().info(__func__,"Worker ID has been set:", globalConfigs.getWorkerId());
                initSender.setFlag();
                packet = create_packet(fwdStack.popForwardStack());
            }
        } else {
            Log().info(__func__, "Initial handshake packet ready");
            packet["head"] = P_HANDSHAKE;
            packet["id"] = "W000";
            handshakeSent.setFlag();
        }
    } else {
        //Get packet to be sent to the server
        packet = create_packet(fwdStack.popForwardStack());
    }

    return packet;
}