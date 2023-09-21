#include <string>

#include "../include/packet.hpp"
#include "../include/process.hpp"
#include "../include/task.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../instance/instance_list.hpp"
#include "../socket/socket.hpp"
#include "../services/sqlite_database_access.hpp"
#include "../instance/instance.hpp"
#include "../data_processor/data_processor.hpp"
#include "../scheduler/task_pool.hpp"
#include "../configs.hpp"
#include "data_parser.hpp"
#include "receiver.hpp"

using nlohmann::json_schema::json_validator;

class Receiver 
{
    private:
        std::uint8_t packetStatus;
        json packet;
    public:
        Receiver(json);
        std::string tableId;
        ReceiverStatus receiverStatus;
        Flag isUserData;
        DataProcessContainer dataProcContainer;

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
      }
    },
    "required": [
                 "head",
                 "id"
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

Receiver::Receiver(json packet)
{
    this->packet = packet;
    isUserData.initFlag(false);
}

// validatePacketHead(): validates the packet and returns the error code depending on output of validator
ReceiverStatus Receiver::validatePacketHead()
{
    json_validator validator;
    validator.set_root_schema(packetSchema);

    try {
        auto defaultPatch = validator.validate(packet);
        Log().info(__func__, "packet head validated.");
        return P_VALID;
    } catch (const std::exception &e) {
        Log().debug(__func__, "packet invalid, ", e.what());
        return P_ERROR;
    }
}

ReceiverStatus Receiver::validatePacketBodyType()
{
    json_validator validator;

    validator.set_root_schema(dataPacketSchema);

    try {
        auto defaultPatch = validator.validate(packet["body"]);
        Log().info(__func__, "received message is a data packet.");
        isUserData.setFlag();
        return P_VALID;
    } catch (const std::exception &e) {
        Log().debug(__func__, "body field is not User data ", e.what());
        
        validator.set_root_schema(instancePacketSchema);
        try{
            auto defaultPatch = validator.validate(packet["body"]);
            Log().info(__func__, "received message is a instance packet.");
            return P_VALID;
        }catch(const std::exception &e){
            Log().debug(__func__, "body field is not Instance data ", e.what());
        }
    }

    Log().error(__func__, "packet body is corrupted!");
    return P_ERROR;
}

/* identify_packet(): internal method that is used for validating/identifying the type of server packet.
 * The receiver process receives the packet and this method is used to first validate the packet, if valid the packet is
 * then identified as what type of packet its is. there are two catagories at the moement acknowldge and data packets.
 * acknowledge packets are acknowledgments for the packets sent by us, data packets are data for processing or node data
 * dent by server. The matchItem is called to match the acknowldge packets in await stack.
 * This method gives exit code for receiver_finalize method.
*/
ReceiverStatus Receiver::identifyPacketType()
{
    ReceiverStatus rc = P_ERROR;
    int packetHead;

    if(validatePacketHead() == P_VALID){
        packetHead = packet["head"];
        if(packetHead & SP_HANDSHAKE){
            try{
                std::string workerId = packet["id"];
                globalConfigs.setWorkerId(workerId);
                return P_EMPTY;
            }catch(std::exception &e){
                Log().error(__func__, "failed to set worker ID");
                return P_ERROR;
            }
        }
        if(packetHead & SP_DATA_SENT){
            if(validatePacketBodyType() != P_ERROR){
                if(isUserData.isFlagSet()){
                    UserDataParser userDataParser(packet);
                    return userDataParser.processDataPacket(tableId, &dataProcContainer);
                } else {
                    InstanceDataParser instanceDataParser(packet);
                    return instanceDataParser.processInstancePacket(tableId);
                }
            }
        }
        if(packetHead & SP_INTR_ACK){
            if(senderSink.matchItemInAwaitStack(SP_INTR_ACK, packet["id"]))
                return P_OK;
            else return P_EMPTY;
        }else if(packetHead & SP_FRES_ACK){
            if(senderSink.matchItemInAwaitStack(SP_FRES_ACK, packet["id"]))
                return P_OK;
            else return P_EMPTY;
        }
    }
    Log().debug(__func__, "invalid header code/packet type detected");
    return rc;
}

JobStatus receiver_proccess(void *data)
{
    Receiver *recv = (Receiver*)data;
    // recv->receiverStatus = recv->identifyPacketType();
    if(recv->identifyPacketType() == P_ERROR)
        return JOB_FAILED;
    return JOB_DONE;
}

void receiver_finalize(void *data)
{
    struct Receiver *recv = (struct Receiver*) data;
    // notify server data received successfully
    Log().info(__func__,"packet received successfully");
    if(recv->isUserData.isFlagSet())
        senderSink.pushPacket("", recv->tableId, DAT_RECVD, recv->dataProcContainer.tData->priority);
    // No need to send data if it was a acknowledge or handshake data
    else if(!recv->tableId.empty())
        senderSink.pushPacket("", instanceList.getInstanceActualName(recv->tableId), DAT_RECVD, DEFAULT_PRIORITY);
    // container should not be derefrenced after this as its dellocated by dataprocessor
    if(recv->isUserData.isFlagSet())
        init_data_processor(recv->dataProcContainer);
    delete recv;
}

void receiver_fail(void *data)
{
    struct Receiver *recv = (struct Receiver*) data;

    // tableID itself is corrupt or it was a status signal that was lost in transmission
    if(recv->tableId.empty()){
        Log().debug(__func__, "packet data fields corrupted, resend packet");
        senderSink.pushPacket("","", RECV_ERR, HIGH_PRIORITY);
    } else {
        Log().debug(__func__, "packet corrupted, resend packet");
        senderSink.pushPacket("", instanceList.getInstanceActualName(recv->tableId), RECV_ERR, HIGH_PRIORITY);
    }
    delete recv;
};

struct ProcessStates* receiver_proc = new ProcessStates {
    .name = "receiver stage",
    .type = RECEIVER_STAGE,
    .start_proc = receiver_proccess,
    .end_proc = receiver_finalize,
    .fail_proc = receiver_fail
};

int init_receiver(json pkt)
{
    Receiver *recv = new Receiver(pkt);
    int rc = 0;

    // Schedule the receiver task on the task pool as non preemtable aka can't be put on hold/pause
    scheduleTask(receiver_proc, (void*)recv, NON_PREEMTABLE);
    return 0;
}