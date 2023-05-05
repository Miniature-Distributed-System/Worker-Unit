#include <string>

#include "receiver.hpp"
#include "../instance/instance_list.hpp"
#include "../socket/socket.hpp"
#include "../sql_access.hpp"
#include "../algorithm/instance.hpp"
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
        std::string insertValueQuery;
        std::string createTableQuery;
        std::string verifyTableQuery;
        std::string dropTableQuery;
        std::uint8_t packetStatus;
        std::string *colHeaders;
        json packet;
        int columns;
        int rows;
        TaskPriority indexOfTaskPriority(int i) { return static_cast<TaskPriority>(i);}
        //DatabaseAccess *dataBaseAccess;
    public:
        Receiver(struct thread_pool*, json);
        ~Receiver();
        std::string tableId;
        ReceiverStatus receiverStatus;
        Flag isUserData;
        struct thread_pool *thread;
        struct data_proc_container *dataProcContainer;

        ReceiverStatus validatePacketHead();
        ReceiverStatus validatePacketBodyType();
        int countCols(std::string data);
        int createSqlCmds(int cols, std::string body);
        int insert_into_table(std::string data, int startIndex);
        void drop_table();
        ReceiverStatus processDataPacket();
        ReceiverStatus processInstancePacket();
        ReceiverStatus identify_packet();
        void constructDataObjects(std::uint8_t priority, std::string algoType);
        void constructInstanceObjects(std::uint8_t algoType);
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

Receiver::Receiver(struct thread_pool* thread, json packet)
{
    this->thread = thread;
    this->packet = packet;
    isUserData.initFlag();
    columns = 0;
    rows = 0;
    // dataBaseAccess = new DatabaseAccess();
}

Receiver::~Receiver()
{
    // delete dataBaseAccess;
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

// countCols(): This internal method returns the total number of columns the table of packet's data field has.
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

/* createSqlCmds(): internal method which creates the sql commands from the information extracted from the packet sent
 * by the server. it creates table creation command and table data insertion template which will be used by 
 * insert_into_table() method for data entry.
*/
int Receiver::createSqlCmds(int cols, std::string body)
{
    int i, start = 0, end = 0;
    createTableQuery = "CREATE TABLE " + tableId + " (";
    insertValueQuery = "INSERT INTO "+ tableId +" (";
    verifyTableQuery = "DESC " + tableId + ";";

    colHeaders = new std::string[cols];
    
    for(i = 0; i < cols; i++)
    {
        while(end < body.length()){
            if(body[end] == ',' || body[end] == '\n')
                break;
            end++;
        }
        std::string colFeild = body.substr(start, end - start);
        createTableQuery.append(colFeild).append(" varchar(30) NOT NULL");
        insertValueQuery.append(colFeild);
        colHeaders[i] = colFeild;
        createTableQuery.append(",");
        start = ++end;
    
        if(i < cols - 1){
            
            insertValueQuery.append(",");
        }
    }

    createTableQuery.append("ID INTEGER PRIMARY KEY AUTOINCREMENT);");
    insertValueQuery.append(") VALUES (");

    return end;
}

// insert_into_table(): This method does data entry into the selected table and appropriate columns.
int Receiver::insert_into_table(std::string data, int startIndex)
{
    int rc, i, end, start;
    std::string tempInsert;

    end = start = startIndex;
    //construct table
    const char *sqlCmd = createTableQuery.c_str();
    rc = dataBaseAccess->writeValue(sqlCmd);
    if(rc != SQLITE_OK){
        std::string *temp = dataBaseAccess->readValue(verifyTableQuery.c_str(), -1);
        if(temp == NULL){
            DEBUG_ERR(__func__, "Failed to insert table into database");
            return EXIT_FAILURE;
        }
    }
    //insert values into table
    while(end < data.length())
    {
        tempInsert = insertValueQuery;
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
        rc = dataBaseAccess->writeValue(sqlInsert);
        if(rc != SQLITE_OK){
            rc = dataBaseAccess->writeValue(sqlInsert);
                if(rc == SQLITE_OK){
                rows++;
            }
        }else {
            rows++;
        }
    }
    DEBUG_MSG(__func__, "total rows inserted into table:",rows);
    return 0;
}

// drop_table(): drops table currently being created. called when there is a failure.
void Receiver::drop_table()
{
    int rc;

    DEBUG_MSG(__func__, "dropping current table in database...");
    dropTableQuery = "DROP TABLE " + tableId + ";";
    //We don't give a knock is it fails as we are already failing at this point
    dataBaseAccess->writeValue(dropTableQuery.c_str());
}

// construct_table(): This method is used for constructing the table and data processor data structure.
void Receiver::constructDataObjects(std::uint8_t priority, std::string algoType)
{
    table *tData;
    tData = new table(rows, columns);
    tData->tableID = tableId;
    tData->instanceType = algoType;
    tData->priority = indexOfTaskPriority(priority);
    dataProcContainer = new data_proc_container(colHeaders, tData);
}

// construct_table(): This method is used for constructing the Instance data strcuture and update the instance list.
void Receiver::constructInstanceObjects(std::uint8_t algoType)
{
    Instance *instance = new Instance(tableId, algoType, columns, rows);
    globalInstanceList.addInstance(instance);
}

/* processDataPacket(): process the packets that are sent to it.
 * it extracts the information from packet fields into local packet varaibles for further pre-processing.
 * It counts the columns and creates the sql commands for inserting into tables.
 * After these are done it finally calls constructDataObjects().
*/
ReceiverStatus Receiver::processDataPacket()
{
    std::string bodyData, algoType;
    int bodyDataStart, priority;

    //Even if one fails we flag error and not proceed further
    try{
        tableId = packet["body"]["tableid"];
        bodyData = packet["body"]["data"];
        algoType = packet["body"]["instancetype"];
        priority = packet["body"]["priority"];
    }catch(json::exception e){
        DEBUG_ERR(__func__, e.what());
        return P_ERROR;
    }

    // Increment counter for instance type
    instanceList.incrimentRefrence(algoType);
    // Get the alias name of the instance thats used by local database here
    algoType = instanceList.getInstanceAliasName(algoType);
    if(algoType.empty()){
        DEBUG_MSG(__func__, "algo type:", packet["body"]["tableid"]," not found");
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
    constructDataObjects(priority, algoType);
    
    return P_SUCCESS;
}

/* processInstancePacket(): process the packets that are sent to it.
 * it extracts the information from packet fields into local packet varaibles for further pre-processing.
 * It counts the columns and creates the sql commands for inserting into tables.
 * After these are done it finally calls constructInstanceObjects.
*/
ReceiverStatus Receiver::processInstancePacket()
{
    std::string bodyData;
    int bodyDataStart, algoType;

    //Even if one fails we flag error and not proceed further
    try{
        tableId = packet["body"]["instanceid"];
        bodyData = packet["body"]["data"];
        algoType = packet["body"]["algotype"];
        
    }catch(json::exception e){
        DEBUG_ERR(__func__, e.what());
        return P_ERROR;
    }

    tableId = instanceList.addInstance(tableId);

    columns = countCols(bodyData);    
    bodyDataStart = createSqlCmds(columns, bodyData);

    //create table and insert into table;
    if(insert_into_table(bodyData, bodyDataStart) == EXIT_FAILURE){
        //drop table before we fail
        drop_table();
        return P_ERROR;
    }

    //initilze and construct table & data_processor bundle
    constructInstanceObjects(algoType);

    return P_SUCCESS;
}

/* identify_packet(): internal method that is used for validating/identifying the type of server packet.
 * The receiver process receives the packet and this method is used to first validate the packet, if valid the packet is
 * then identified as what type of packet its is. there are two catagories at the moement acknowldge and data packets.
 * acknowledge packets are acknowledgments for the packets sent by us, data packets are data for processing or node data
 * dent by server. The matchItemWithAwaitStack is called to match the acknowldge packets in await stack.
 * This method gives exit code for receiver_finalize method.
*/
ReceiverStatus Receiver::identify_packet()
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
                // if(!dataBaseAccess->initDatabase()){ 
                // }
                if(isUserData.isFlagSet()){
                    rc = processDataPacket();
                } else  rc = processInstancePacket();
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

struct process* receiver_proc = new process {
    .start_proc = receiver_proccess,
    .end_proc = receiver_finalize
};

int init_receiver(struct thread_pool* thread, json pkt)
{
    Receiver *recv = new Receiver(thread, pkt);
    int rc = 0;

    //DEBUG_MSG(__func__, "init receiver");
    //Schedule the receiver task on the task pool
    scheduleTask(thread, receiver_proc, (void*)recv, 0);
    return 0;
}