#include "../include/debug_rp.hpp"
#include "../instance/instance_list.hpp"
#include "../include/process.hpp"
#include "../services/sqlite_database_access.hpp"
#include "../include/logger.hpp"
#include "data_parser.hpp"

TaskPriority indexOfTaskPriority(int i) { return static_cast<TaskPriority>(i);}

void UserDataParser::constructDataObjects(std::uint8_t priority, std::string algoType)
{
    TableData *tData;
    // The first row is the Column name rows aka header
    tData = new TableData(rows - 1, columns);
    tData->tableID = tableId;
    tData->instanceType = algoType;
    tData->priority = indexOfTaskPriority(priority);
    *dataProcContainer = DataProcessContainer(colHeaders, tData);
}

ReceiverStatus UserDataParser::processDataPacket(std::string &tableID, DataProcessContainer *dataProcessContainer)
{
    std::string bodyData, algoType;
    int bodyDataStart, priority;
    this->dataProcContainer = dataProcessContainer;

    if(UserdataPacketContainer(*packet).getBody(tableId, algoType, priority, bodyData))
        return P_ERROR;

    try{
        fileDataBaseAccess = new FileDataBaseAccess(tableId, RW_FILE);
    } catch(std::exception &e){
        Log().error(__func__, e.what());
        return P_ERROR;
    }

    tableID = tableId;
	DataBaseInstanceList &instanceList = globalObjectsManager.get<DataBaseInstanceList>();
    // Increment counter for instance type
    instanceList.incrimentRefrence(algoType);
    // Get the alias name of the instance thats used by local database here
    algoType = instanceList.getInstanceAliasName(algoType);
    if(algoType.empty()){
        Log().debug(__func__, "instance type:",
			[&](std::string &templateType) {
				UserdataPacketContainer(*packet).getTemplateType(templateType);
				return templateType;
			}," not found");
        return P_ERROR;
    }

    fileDataBaseAccess->writeBlob(bodyData);
    columns = fileDataBaseAccess->getTotalColumns();
    rows = fileDataBaseAccess->getTotalRows();
    Log().info(__func__, "rows:", rows, " columns:",columns);

    //initilze and construct table & data_processor bundle
    constructDataObjects(priority, algoType);
    delete fileDataBaseAccess;
    
    return P_SUCCESS;
}
