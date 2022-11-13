#include <iostream>
#include <string>
#include <thread>
#include <sqlite3.h> 
#include "ws_client.hpp"
#include "packet.hpp"
#define ERR_DBOPEN -1

json packet;
sqlite3 *db;
std::string tableId = "";
std::string insertCmd, createCmd, dropCmd;

void resetPacket(){
    //Set handshake bit which indicates that soc conn is made
    //for the first time and server needs to be notified of 
    //new client
    packet["head"] = P_RESET;
    //Send cuurent benchmark data(unimpliemented as of now)
    packet["id"] = "";
    packet["body"] = "";
}

int countCols(std::string data){
    int i = -1;
    int rows = 0;
    while(data[i++] != '\n'){
        if(data[i] == ',')
            rows++;
    }
    //count the last row as it doesn't end with ','
    return rows + 1;
}

int createSqlCmds(int cols, std::string header){
    int i, start = 0, end = 0;
    createCmd = "CREATE TABLE " + tableId + "(";
    insertCmd = "INSERT INTO "+ tableId +"(";

    for(i = 0; i < cols; i++)
    {
        while(header[end] != ',' || header[end] != '\n'){
            end++;
        }
        std::string colFeild = header.substr(start, end - 1);
        createCmd.append(colFeild).append(" varchar(30) NOT NULL");
        insertCmd.append(colFeild);
        createCmd.append(",");
        start = end;
    
        if(i < cols - 1){
            
            insertCmd.append(",");
        }
    }

    createCmd.append("id int NOT NULL AUTO_INCREMENT, PRIMARY KEY(id));");
    insertCmd.append(") VALUES(");
    
    return 0;
}

int insertIntoTable(std::string data, int cols){
    char* sqlErrMsg;
    int rc, i, end = 0, start = 0;
    std::string tempInsert;

    //construct table
    const char *sqlCmd = createCmd.c_str();
    rc = sqlite3_exec(db, sqlCmd,NULL , 0, &sqlErrMsg);
    if(rc != SQLITE_OK){
            sqlite3_free(sqlErrMsg);
            return EXIT_FAILURE;
    }

    //insert values into table
    while(end < data.length())
    {
        tempInsert = insertCmd;
        //iterate through one row at a time
        for(i = 0; i < cols; i++)
        {
            while(data[end] != ',' || data[end] != '\n')
            {
                end++;
            }
            //get single row value at a time
            tempInsert.append("'" + data.substr(start, end - 1) + "'");
            if(data[end] != '\n')
                tempInsert.append(",");
            start = end;
        }
        //finish insert command
        tempInsert.append(");");

        const char *sqlInsert = tempInsert.c_str();
        rc = sqlite3_exec(db, sqlInsert,NULL , 0, &sqlErrMsg);
        if(rc != SQLITE_OK){
                sqlite3_free(sqlErrMsg);
                return EXIT_FAILURE;
        }
    }

    return 0;
}

void dropTable(){
    const char *sqlCmd;
    char* sqlErrMsg;
    int rc;

    dropCmd = "DROP TABLE " + tableId + ";";
    sqlCmd = dropCmd.c_str();
    sqlite3_exec(db, sqlCmd,NULL , 0, &sqlErrMsg);
}

int processPacket()
{
    std::string bodyData = packet["body"];
    tableId = packet["id"];
    int rc = 0, cols;

    //empty the packet
    resetPacket();

    //Count the rows in csv
    cols = countCols(bodyData);
    rc = createSqlCmds(cols, bodyData);

    rc = sqlite3_open("csv.db", &db);
    if(rc){
        return ERR_DBOPEN;
    }
    //create table and insert into table;
    rc = insertIntoTable(bodyData, cols);
    if(rc == EXIT_FAILURE){
        //drop table before we fail
        dropTable();
    }
    packet["id"] = tableId;
    sqlite3_close(db);

    return rc;
}

void *receiver_process(void *ptr){
    int ret = 0;
    char* args = (char*)ptr;
    std::string port = (char*)args[0];
    std::string hostname = (char*)args[1];
    
    //init packet
    resetPacket();
    //set packet as initial handshake
    packet["head"] = P_HANDSHAKE;

    while(1){
        ret = ws_client_launch(port, hostname);
        if(ret == EXIT_FAILURE){
            //need to report back crash to server
            break;
        }
        ret = processPacket();
        if(ret == ERR_DBOPEN){
            //need to report back crash to server
            packet["head"] = P_SEIZE;
            ws_client_launch(port, hostname);
            break;
        }else if(ret == EXIT_FAILURE){
            //notufy server to resend data
            packet["head"] = P_RSEND_DATA;
        }else{
            //notify server data received successfully
            packet["head"] = P_RECV_DATA;
        }
        
    }

    return 0;
}

int main(int argc, char** argv)
{
    pthread_t mainThread;
    std::string args[2];
    int ret;
    
    if(argc != 3){
        args[0] = "8080";
        args[1] = "0:0:0:0";
    }
    else{
        args[0] = argv[1];
        args[1] = argv[2];
    }

    ret = pthread_create(&mainThread, NULL, receiver_process, (void*)args);
    return 0;
}