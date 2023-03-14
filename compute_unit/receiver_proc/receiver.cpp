#include <string>

#include "receiver.hpp"
#include "../socket/socket.hpp"
#include "../sql_access.hpp"
#include "../data_processor.hpp"
#include "../include/packet.hpp"
#include "../include/process.hpp"
#include "../include/task.hpp"
#include "../include/debug_rp.hpp"

using nlohmann::json_schema::json_validator;

//These are local status codes
enum ReceiverStatus{
    P_EMPTY = 0,
    P_OK,
    SND_RESET,
    P_ERROR,
    P_DAT_ERR,
    P_SUCCESS,
    P_VALID,
};

class Receiver 
{
    private:
        std::string insertCmd;
        std::string createCmd;
        std::string dropCmd;
        std::uint8_t packetStatus;
        std::string *colHeaders;
        json packet;
        int columns;
        int rows;
        TaskPriority indexOfTaskPriority(int i) { return static_cast<TaskPriority>(i);}
    public:
        Receiver(struct thread_pool*, json);
        std::string tableId;
        ReceiverStatus receiverStatus;
        struct thread_pool *thread;
        struct data_proc_container *dataProcContainer;

        ReceiverStatus validatePacket();
        int countCols(std::string data);
        int createSqlCmds(int cols, std::string body);
        int insert_into_table(std::string data, int startIndex);
        void drop_table();
        ReceiverStatus process_packet();
        ReceiverStatus identify_packet();
        void construct_table(std::uint8_t priority, std::uint8_t algoType);
};

static json packetSchema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "A person",
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
        "type": "object",
        "properties":{
          "tableid":{
            "type": "string"
          },
          "priority":{
            "type": "number",
            "minimum": 0,
            "maximum": 6
          },
          "algotype":{
            "type": "number",
            "minimum": 0,
            "maximum": 10
          },
          "data":{
            "type": "string"
          }
        }
      }
    },
    "required": [
                 "head",
                 "id"
                 ],
    "type": "object"
}
)"_json;

Receiver::Receiver(struct thread_pool* thread, json packet)
{
    this->thread = thread;
    this->packet = packet;
    columns = 0;
    rows = 0;
}

{
    json_validator validator;
    validator.set_root_schema(packetSchema);

    try {
        auto defaultPatch = validator.validate(packet);
        DEBUG_MSG(__func__, "packet validation success!");
        return P_VALID;
    } catch (const std::exception &e) {
        DEBUG_ERR(__func__, "packet invalid, ", e.what());
        return P_ERROR;
    }
}

int Receiver::countCols(std::string data){
    int i = -1;
    int cols = 0;
    while(data[i++] != '\n'){
        if(data[i] == ',')
            cols++;
    }
    //count the last col as it doesn't end with ','
    DEBUG_MSG(__func__, "number of columns:", cols+1);
    return cols + 1;
}

int Receiver::createSqlCmds(int cols, std::string body)
{
    int i, start = 0, end = 0;
    createCmd = "CREATE TABLE " + tableId + " (";
    insertCmd = "INSERT INTO "+ tableId +" (";
    colHeaders = new std::string[cols];
    
    for(i = 0; i < cols; i++)
    {
        while(end < body.length()){
            if(body[end] == ',' || body[end] == '\n')
                break;
            end++;
        }
        std::string colFeild = body.substr(start, end - start);
        createCmd.append(colFeild).append(" varchar(30) NOT NULL");
        insertCmd.append(colFeild);
        colHeaders[i] = colFeild;
        createCmd.append(",");
        start = ++end;
    
        if(i < cols - 1){
            
            insertCmd.append(",");
        }
    }

    createCmd.append("ID INTEGER PRIMARY KEY AUTOINCREMENT);");
    insertCmd.append(") VALUES (");

    return end;
}

int Receiver::insert_into_table(std::string data, int startIndex)
{
    int rc, i, end, start;
    std::string tempInsert;

    end = start = startIndex;
    //construct table
    const char *sqlCmd = createCmd.c_str();
    sql_write(sqlCmd);
    DEBUG_MSG(__func__,"\n");
    //insert values into table
    while(end < data.length())
    {
        tempInsert = insertCmd;
        //iterate through one col at a time
        for(i = 0; i < columns; i++)
        {
             while(end < data.length()){
                if(data[end] == ',' || data[end] == '\n')
                    break;
                end++;
            }
            //get single row value at a time
            tempInsert.append("'" + data.substr(start, end - start) + "'");
            if(i < columns - 1)
                tempInsert.append(",");
            start = ++end;
        }
        //finish insert command
        tempInsert.append(");");
        //DEBUG_MSG(__func__, "constructed insert cmd:", tempInsert);
        const char *sqlInsert = tempInsert.c_str();
        rc = sql_write(sqlInsert);
        if(rc != SQLITE_OK){
            rc = sql_write(sqlInsert);
                if(rc == SQLITE_OK){
                DEBUG_MSG(__func__, "nos rows into table:",rows);
                rows++;
            }
        }else {
            DEBUG_MSG(__func__, "nos rows into table:",rows);
            rows++;
        }
    }

    return 0;
}

void Receiver::drop_table()
{
    int rc;

    DEBUG_MSG(__func__, "dropping current table in database...");
    dropCmd = "DROP TABLE " + tableId + ";";
    //We don't give a knock is it fails as we are already failing at this point
    sql_write(dropCmd.c_str());
}

void Receiver::construct_table(std::uint8_t priority, std::uint8_t algoType)
{
    struct table *tData;
    tData = new table(rows, columns);
    tData->tableID = tableId;
    tData->algorithmType = algoType;
    tData->priority = indexOfTaskPriority(priority);
    dataProcContainer = new data_proc_container;
    dataProcContainer->tData = tData;
    dataProcContainer->colHeaders = colHeaders; 
}

{
    std::string bodyData;
    int bodyDataStart, priority, algoType;

    //Even if one fails we flag error and not proceed further
    try{
        tableId = packet["body"]["tableid"];
        bodyData = packet["body"]["data"];
        algoType = packet["body"]["algotype"];
        priority = packet["body"]["priority"];
    }catch(json::exception e){
        DEBUG_ERR(__func__, e.what());
        return P_ERROR;
    }

    columns = countCols(bodyData);    
    bodyDataStart = createSqlCmds(columns, bodyData);

    //create table and insert into table;
    if(insert_into_table(bodyData, bodyDataStart) == EXIT_FAILURE){
        //drop table before we fail
        drop_table();
        return P_ERROR;
    }

    //initilze and construct table & data_processor bundle
    construct_table(priority, algoType);

    return P_SUCCESS;
}

ReceiverStatus Receiver::identify_packet()
{
    ReceiverStatus rc;
    int packetHead;

    if(validatePacket() == P_VALID){
        packetHead = packet["head"];
        if(packetHead & SP_HANDSHAKE){
            DEBUG_MSG(__func__, "Handshake");
            computeID = packet["id"];
            rc = P_EMPTY;
        }
        if(packetHead & SP_DATA_SENT){
            rc = process_packet();
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
        if(rc == P_VALID){
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
    recv->receiverStatus = recv->identify_packet();
    //if(recv->receiverStatus == EXIT_FAILURE){
    //    return JOB_FAILED;
    //}
    return JOB_DONE;
}

JobStatus receiver_finalize(void *data, JobStatus signal)
{
    struct Receiver *recv = (struct Receiver*) data;
    
    if(recv->receiverStatus == P_ERROR){
        /* tableID itself is corrupt or it was a status signal that was lost 
           in transmission */
        if(recv->tableId.empty()){
            DEBUG_ERR(__func__, "packet data fields corrupted, resend packet");
            send_packet("","", RECV_ERR, HIGH_PRIORITY);
        } else {
            DEBUG_ERR(__func__, "packet corrupted, resend packet");
            send_packet("", recv->tableId, RECV_ERR, HIGH_PRIORITY);
        }
    } else if(recv->receiverStatus == P_SUCCESS){
        //notify server data received successfully
        DEBUG_MSG(__func__,"packet received successfully");
        send_packet("", recv->tableId, DAT_RECVD, DEFAULT_PRIORITY);
        /*container should not be derefrenced after this as its dellocated 
          by dataprocessor */
        init_data_processor(recv->thread, recv->dataProcContainer);
    }
    /* if neither of the cases where true then no need to send any respone to
       server. empty packet is sent by default if fwd stack is empty else
       queued packets are sent out to server. */
    delete recv;
    
    return JOB_FINISHED;
}

struct process* receiver_proc = new process {
    .start_proc = receiver_proccess,
    .end_proc = receiver_finalize
};

int init_receiver(struct thread_pool* thread, json pkt)
{
    Receiver *recv = new Receiver(thread, pkt);
    int rc = 0;

    DEBUG_MSG(__func__, "init receiver");
    //this task takes higher priority than all
    scheduleTask(thread, receiver_proc, (void*)recv, 0);
    return 0;
}