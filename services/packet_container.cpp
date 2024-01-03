#include "packet_container.hpp"
#include "../include/logger.hpp"

int PacketContainer::getHead(int &head)
{
	try {
		head = packet[PK_HEAD];
	} catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

int PacketContainer::getId(std::string &id)
{
	try {
        id = packet[PK_ID];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int PacketContainer::getDataId(std::string &dataId)
{
	// Nothing to impliment here therefore return false everytime 
	return -1;
}

int UserdataPacketContainer::getDataId(std::string &dataId)
{
	try {
        dataId = packet[PK_BODY][UD_PK_DATAID];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int TemplatePacketContainer::getDataId(std::string &dataId)
{
	try {
        dataId = packet[PK_BODY][TP_PK_DATAID];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
   
    return 0;

}

int UserdataPacketContainer::getPriority(int &priority)
{
	try {
        priority = packet[PK_BODY][UD_PK_PRIORITY];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int UserdataPacketContainer::getTemplateType(std::string &templateType)
{
	try {
        templateType = packet[PK_BODY][UD_PK_TEMPLATETYPE];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int TemplatePacketContainer::getTemplateId(std::string &instanceId)
{
	try {
        instanceId = packet[PK_BODY][TP_PK_DATAID];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int TemplatePacketContainer::getAlgorithmType(int &algoType)
{
	try {
        algoType = packet[PK_BODY][TP_PK_ALGOTYPE];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

int PacketContainer::getData(std::string &data)
{
	try {
        data = packet[PK_BODY][PK_DATA];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int UserdataPacketContainer::getBody(std::string &tableId, std::string &instanceType, int &priority, std::string &data)
{
	try {
        tableId = packet[PK_BODY][UD_PK_DATAID];
        data = packet[PK_BODY][PK_DATA];
        instanceType = packet[PK_BODY][UD_PK_TEMPLATETYPE];
        priority = packet[PK_BODY][UD_PK_PRIORITY];
	} catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

int TemplatePacketContainer::getBody(std::string &instanceId, int &algoType, std::string &data)
{
	try {
        instanceId = packet[PK_BODY][TP_PK_DATAID];
        data = packet[PK_BODY][PK_DATA];
		algoType = packet[PK_BODY][TP_PK_ALGOTYPE];		
	} catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

void PacketContainer::setHead(int head)
{
	packet[PK_HEAD] = head;
}

void PacketContainer::setId(std::string id)
{
	packet[PK_ID] = id.c_str();
}

void PacketContainer::setTableid(std::string tableId)
{
	packet[PK_BODY][PK_ID] = tableId.c_str();
}

void PacketContainer::setPriority(int priority)
{
	packet[PK_BODY][UD_PK_PRIORITY] = priority;
}

void PacketContainer::setData(std::string &data)
{
	packet[PK_BODY][PK_DATA] = data.c_str();
}

void PacketContainer::setStats(nlohmann::json statsJson)
{
	packet[PK_STATS] = statsJson;
}
