#include <string>

#include "receiver.hpp"
#include "../socket/socket.hpp"
#include "../sender_proc/sender.hpp"
#include "../sql_access.hpp"
#include "../data_processor.hpp"
#include "../include/packet.hpp"
#include "../include/process.hpp"
#include "../include/task.hpp"
#include "../include/debug_rp.hpp"

std::string tableId = "";
std::string insertCmd, createCmd, dropCmd;
struct data_proc_container *dataProcContainer;
using nlohmann::json_schema::json_validator;

enum receive_stat{
    SND_EMPTY_DAT = 0,
    SND_RESET,
    P_ERR,
    P_SUCCESS,
};

int countCols(std::string data){
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
        
    public:
        Receiver(struct thread_pool*, json);
        std::string tableId;
        std::uint8_t receiverStatus;
        struct thread_pool *thread;
        struct data_proc_container *dataProcContainer;

        int validatePacket();
        int countCols(std::string data);
        int createSqlCmds(int cols, std::string body);
        int insert_into_table(std::string data, int startIndex);
        void drop_table();
        int process_packet();
        int identify_packet();
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

int Receiver::validatePacket()
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
        if(rc == SQLITE_OK){
            DEBUG_MSG(__func__, "nos rows into table:",dataProcContainer->rows);
            dataProcContainer->rows++;
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
    tData->priority = priority;
    dataProcContainer = new data_proc_container;
    dataProcContainer->tData = tData;
    dataProcContainer->colHeaders = colHeaders; 
}

int Receiver::process_packet()
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
        return EXIT_FAILURE;
    }

    //initilze and construct table & data_processor bundle
    construct_table(priority, algoType);

    return P_SUCCESS;
}

int Receiver::identify_packet()
{
    int rc;
    int packetHead;

    try{
        packetHead = recv->packet["head"];
    }catch(json::exception e){
        DEBUG_ERR(__func__, e.what());
        return P_ERR;
    }
    {
        case P_HANDSHAKE:
            computeID = recv->packet["id"];
            if((packetHead & (P_HANDSHAKE | P_DATSENT))){
                DEBUG_MSG(__func__, "only handshake done");
                rc = SND_EMPTY_DAT;
                break;
            }
        case P_DATSENT:
            DEBUG_MSG(__func__, "process packet");
            rc = process_packet(recv);
            break;
        case P_RESET:
        default:
            rc = SND_RESET;
    }

    return rc;
}

int receiver_proccess(void *data)
{
    Receiver *recv = (Receiver*)data;
    recv->receiverStatus = recv->identify_packet();
    //if(recv->receiverStatus == EXIT_FAILURE){
    //    return JOB_FAILED;
    //}
    return JOB_DONE;
}

int receiver_finalize(void *data)
{
    struct receiver *recv = (struct receiver*) data;
    
    if(recv->receiverStatus == SND_EMPTY_DAT)
    {
        DEBUG_MSG(__func__, "sending empty packet");
        send_packet("","", 100);
    } 
    else if(recv->receiverStatus == P_ERR) {
        //tableID itself is corrput or it was a status signal that was lost in transmission
        if(recv->tableID.empty())
        {
            DEBUG_MSG(__func__, "packet corrupt");
            send_packet("","", RECV_ERR, HEAVY_LOAD);
        } else {
            //notify server to resend data
            DEBUG_ERR(__func__, "packet error encountered, resend packet");
            send_packet("", recv->tableID, PROC_ERR);
            send_packet("", recv->tableId, RECV_ERR, HEAVY_LOAD);
        }
        
    } else {
        //notify server data received successfully
        DEBUG_MSG(__func__,"packet received successfully");
        //container should be derefrenced after this as its deleted by dataprocessor
        init_data_processor(recv->thread, recv->container);
        send_packet("", recv->tableId, DAT_RECVD, DEFAULT_LOAD);
    }
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
    sched_task(thread, receiver_proc, (void*)recv, 0);
    return 0;
}