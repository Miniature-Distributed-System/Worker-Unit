#include "../include/debug_rp.hpp"
#include "../algorithm/instance.hpp"
#include "../instance/instance_list.hpp"
#include "data_parser.hpp"
#include "../services/sqlite_database_access.hpp"

int countColumns(std::string data){
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

int InstanceDataParser::createSqlCmds(int cols, std::string body)
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

int InstanceDataParser::insertIntoTable(std::string data, int startIndex)
{
    int rc, i, end, start;
    std::string tempInsert;
    rows = 0;

    end = start = startIndex;
    //construct table
    const char *sqlCmd = createTableQuery.c_str();
    rc = sqliteDatabaseAccess->writeValue(sqlCmd);
    if(rc != SQLITE_OK){
        std::string *temp = sqliteDatabaseAccess->readValue(verifyTableQuery.c_str(), -1);
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
        rc = sqliteDatabaseAccess->writeValue(sqlInsert);
        if(rc != SQLITE_OK){
            rc = sqliteDatabaseAccess->writeValue(sqlInsert);
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
void InstanceDataParser::dropTable()
{
    int rc;

    DEBUG_MSG(__func__, "dropping current table in database...");
    dropTableQuery = "DROP TABLE " + tableId + ";";
    //We don't give a knock is it fails as we are already failing at this point
    sqliteDatabaseAccess->writeValue(dropTableQuery.c_str());
}

void InstanceDataParser::constructInstanceObjects(std::uint8_t algoType)
{
    Instance *instance = new Instance(tableId, algoType, columns, rows);
    globalInstanceList.addInstance(instance);
    DEBUG_MSG(__func__, "added instance to list");
}

ReceiverStatus InstanceDataParser::processInstancePacket(std::string &tableID)
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
    
    tableID = tableId = instanceList.addInstance(tableId);

    columns = countColumns(bodyData);    
    bodyDataStart = createSqlCmds(columns, bodyData);

    DEBUG_MSG(__func__, "inserting into: ", tableId);
    //create table and insert into table;
    if(insertIntoTable(bodyData, bodyDataStart) == EXIT_FAILURE){
        //drop table before we fail
        dropTable();
        return P_ERROR;
    }

    //initilze and construct table & data_processor bundle
    constructInstanceObjects(algoType);
    DEBUG_MSG(__func__, "instance data parsing done for: ", tableId);
    
    return P_SUCCESS;
}