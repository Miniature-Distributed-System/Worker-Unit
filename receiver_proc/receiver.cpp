#include <string>

#include "../instance/instance_list.hpp"
#include "../socket/socket.hpp"
#include "../sql_access.hpp"
#include "../algorithm/instance.hpp"
#include "../data_processor/data_processor.hpp"
#include "../include/packet.hpp"
#include "../include/process.hpp"
#include "../include/task.hpp"
#include "../include/debug_rp.hpp"
#include "data_parser.hpp"
#include "receiver.hpp"

using nlohmann::json_schema::json_validator;

class Receiver 
{
    private:
        std::uint8_t packetStatus;
        json packet;
    public:
        Receiver(struct ThreadPool*, json);
        std::string tableId;
        ReceiverStatus receiverStatus;
        Flag isUserData;
        struct ThreadPool *thread;
        struct DataProcessContainer *dataProcContainer;

        ReceiverStatus validatePacketHead();
        ReceiverStatus validatePacketBodyType();
        ReceiverStatus identifyPacketType();
};

static json packetSchema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "Main packet",
    "properties": {
      "head": {
          "description": "packet status",
          "type": "number"
      },
      "id": {
          "description": "CU identification number",
          "type": "string"
      },
      "body":{
        "description": "data holder object",
        "type": "object",
        "properties": {
        }
      }
    },
    "required": [
                 "head",
                 "id",
                 "body"
                 ],
    "type": "object"
}
)"_json;

static json dataPacketSchema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "Data Body of packet",
    "properties": {
      "tableid": {
          "description": "id of table",
          "type": "string"
      },
      "priority": {
          "description": "priority of data",
          "minimum": 0,
          "maximum": 3
      },
      "instancetype": {
          "description": "type of instance",
          "type": "string"
      },
      "data": {
          "description": "data",
          "type": "string"
      }
    },
    "required": [
                 "tableid",
                 "priority",
                 "instancetype",
                 "data"
                 ],
    "type": "object"
}
)"_json;

static json instancePacketSchema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "Data Body of packet",
    "properties": {
      "instanceid": {
          "description": "id of table",
          "type": "string"
      },
      "algotype": {
          "description": "priority of data",
          "minimum": 0,
          "maximum": 3
      },
      "data": {
          "description": "priority of data",
          "type": "string"
      }
    },
    "required": [
                 "instanceid",
                 "algotype",
                 "data"
                 ],
    "type": "object"
}
)"_json;

Receiver::Receiver(struct ThreadPool* thread, json packet)
{
    this->thread = thread;
    this->packet = packet;
    isUserData.initFlag();
}

// validatePacketHead(): validates the packet and returns the error code depending on output of validator
ReceiverStatus Receiver::validatePacketHead()
{
    json_validator validator;
    validator.set_root_schema(packetSchema);

    try {
        auto defaultPatch = validator.validate(packet);
        DEBUG_MSG(__func__, "packet head validated.");
        return P_VALID;
    } catch (const std::exception &e) {
        DEBUG_ERR(__func__, "packet invalid, ", e.what());
        return P_ERROR;
    }
}

ReceiverStatus Receiver::validatePacketBodyType()
{
    json_validator validator;

    validator.set_root_schema(dataPacketSchema);

    try {
        auto defaultPatch = validator.validate(packet["body"]);
        DEBUG_MSG(__func__, "this is a data packet.");
        isUserData.setFlag();
        return P_VALID;
    } catch (const std::exception &e) {
        DEBUG_ERR(__func__, "body field is not User data", e.what());
    }

    validator.set_root_schema(instancePacketSchema);
    try{
        auto defaultPatch = validator.validate(packet["body"]);
        DEBUG_MSG(__func__, "this is a instance packet.");
        return P_VALID;
    }catch(const std::exception &e){
        DEBUG_ERR(__func__, "body field is not Instance data", e.what());
    }
    DEBUG_ERR(__func__, "packet body is corrupted!");
    return P_ERROR;
}

/* identify_packet(): internal method that is used for validating/identifying the type of server packet.
 * The receiver process receives the packet and this method is used to first validate the packet, if valid the packet is
 * then identified as what type of packet its is. there are two catagories at the moement acknowldge and data packets.
 * acknowledge packets are acknowledgments for the packets sent by us, data packets are data for processing or node data
 * dent by server. The matchItemWithAwaitStack is called to match the acknowldge packets in await stack.
 * This method gives exit code for receiver_finalize method.
*/
ReceiverStatus Receiver::identifyPacketType()
{
    ReceiverStatus rc = P_ERROR;
    int packetHead;

    if(validatePacketHead() == P_VALID){
        packetHead = packet["head"];
        if(packetHead & SP_HANDSHAKE){
            DEBUG_MSG(__func__, "Handshake");
            computeID = packet["id"];
            rc = P_EMPTY;
        }
        if(packetHead & SP_DATA_SENT){
            if(validatePacketBodyType() != P_ERROR){
                if(isUserData.isFlagSet()){
                   UserDataParser userDataParser(packet);
                   rc = userDataParser.processInstancePacket(tableId, &dataProcContainer);
                } else {
                    InstanceDataParser instanceDataParser(packet);
                    rc = instanceDataParser.processDataPacket(tableId);
                }
            }
        }
        if(packetHead & SP_INTR_ACK){
            if(awaitStack.matchItemWithAwaitStack(SP_INTR_ACK, packet["id"]))
                rc = P_OK;
            else rc = P_EMPTY;
        }else if(packetHead & SP_FRES_ACK){
            if(awaitStack.matchItemWithAwaitStack(SP_FRES_ACK, packet["id"]))
                rc = P_OK;
            else rc = P_EMPTY;
        }
        if(rc == P_ERROR){
            /*If rc value did not change since entering this block then the 
              header code does not match any of the above and is unknown so drop 
              that packet. */

            //TO-DO: This needs reimplimentaion at server level
            DEBUG_ERR(__func__,"invalid packet header code");
            rc = P_ERROR;
        }
    }
    return rc;
}

JobStatus receiver_proccess(void *data)
{
    Receiver *recv = (Receiver*)data;
    recv->receiverStatus = recv->identifyPacketType();

    return JOB_DONE;
}

JobStatus receiver_finalize(void *data, JobStatus signal)
{
    struct Receiver *recv = (struct Receiver*) data;
    
    if(recv->receiverStatus == P_ERROR){
        // tableID itself is corrupt or it was a status signal that was lost in transmission
        if(recv->tableId.empty()){
            DEBUG_ERR(__func__, "packet data fields corrupted, resend packet");
            send_packet("","", RECV_ERR, HIGH_PRIORITY);
        } else {
            DEBUG_ERR(__func__, "packet corrupted, resend packet");
            send_packet("", instanceList.getInstanceActualName(recv->tableId), RECV_ERR, HIGH_PRIORITY);
        }
    } else if(recv->receiverStatus == P_SUCCESS){
        // notify server data received successfully
        DEBUG_MSG(__func__,"packet received successfully");
        if(recv->isUserData.isFlagSet())
            send_packet("", recv->tableId, DAT_RECVD, DEFAULT_PRIORITY);
        else
            send_packet("", instanceList.getInstanceActualName(recv->tableId), DAT_RECVD, DEFAULT_PRIORITY);
        // container should not be derefrenced after this as its dellocated by dataprocessor
        if(recv->isUserData.isFlagSet())
            init_data_processor(recv->thread, recv->dataProcContainer);
    }
    /* if neither of the cases where true then no need to send any respone to server. empty packet is sent by default if 
       fwd stack is empty else queued packets are sent out to server. */
    delete recv;
    
    return JOB_FINISHED;
}

struct ProcessStates* receiver_proc = new ProcessStates {
    .start_proc = receiver_proccess,
    .end_proc = receiver_finalize
};

int init_receiver(struct ThreadPool* thread, json pkt)
{
    Receiver *recv = new Receiver(thread, pkt);
    int rc = 0;

    //DEBUG_MSG(__func__, "init receiver");
    //Schedule the receiver task on the task pool
    scheduleTask(thread, receiver_proc, (void*)recv, 0);
    return 0;
}