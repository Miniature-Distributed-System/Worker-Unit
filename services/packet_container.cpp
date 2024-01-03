#include "packet_container.hpp"
#include "../include/logger.hpp"

int PacketContainer::getHead(int &head)
{
	try {
		head = packet["head"];
	} catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

int PacketContainer::getId(std::string &id)
{
	try {
        id = packet["id"];
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
        dataId = packet["body"]["tableid"];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int TemplatePacketContainer::getDataId(std::string &dataId)
{
	try {
        dataId = packet["body"]["instanceid"];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
   
    return 0;

}

int UserdataPacketContainer::getPriority(int &priority)
{
	try {
        priority = packet["body"]["priority"];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int UserdataPacketContainer::getTemplateType(std::string &templateType)
{
	try {
        templateType = packet["body"]["instancetype"];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int TemplatePacketContainer::getTemplateId(std::string &instanceId)
{
	try {
        instanceId = packet["body"]["instanceid"];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int TemplatePacketContainer::getAlgorithmType(int &algoType)
{
	try {
        algoType = packet["body"]["algotype"];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

int PacketContainer::getData(std::string &data)
{
	try {
        data = packet["body"]["data"];
    } catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }

	return 0;
}

int UserdataPacketContainer::getBody(std::string &tableId, std::string &instanceType, int &priority, std::string &data)
{
	try {
		tableId = packet["body"]["tableid"];
        data = packet["body"]["data"];
        instanceType = packet["body"]["instancetype"];
        priority = packet["body"]["priority"];
	} catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

int TemplatePacketContainer::getBody(std::string &instanceId, int &algoType, std::string &data)
{
	try {
		instanceId = packet["body"]["instanceid"];
        data = packet["body"]["data"];
        algoType = packet["body"]["algotype"];		
	} catch(nlohmann::json::exception e){
        Log().info(__func__, e.what());
        return -1;
    }
	return 0;
}

void PacketContainer::setHead(int head)
{
	packet["head"] = head;
}

void PacketContainer::setId(std::string id)
{
	packet["id"] = id.c_str();
}

void PacketContainer::setTableid(std::string tableId)
{
	packet["body"]["id"] = tableId.c_str();
}

void PacketContainer::setPriority(int priority)
{
	packet["body"]["priority"] = priority;
}

void PacketContainer::setData(std::string &data)
{
	packet["body"]["data"] = data.c_str();
}

void PacketContainer::setStats(nlohmann::json statsJson)
{
	packet["stats"] = statsJson;
}
