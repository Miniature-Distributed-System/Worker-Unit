#ifndef PKT_CNT_H_
#define PKT_CNT_H
#include "../include/json.hpp"

// These macros define the keys for json data access, don't use the keys directly.
#define PK_HEAD "head"
#define PK_ID "id"
#define PK_BODY "body"
#define PK_DATA "data"
#define PK_STATS "stats"
#define PK_CRC "crc"
#define UD_PK_DATAID "tableid"
#define UD_PK_TEMPLATETYPE "instancetype"
#define UD_PK_PRIORITY "priority"
#define TP_PK_DATAID "instanceid"
#define TP_PK_ALGOTYPE "algotype"

/* 
   PacketContainer: This class is a wrapper for the nlohmann json objects, using the objects
   as is gonna be a maintenance nightmare especially the usage of constant literals (keys) 
   to access data. There is a lot of room to commit errors and handling the exceptions is another 
   chore. we are nuking all of that with this wrapper. While the wrapper adds a layer of 
   performance overhead we are skipping a lot of lines of code and reducing debug times. 
   
   PLEASE only use these wrappers from now on.
*/
class PacketContainer {
		nlohmann::json &packet;
    public:
        PacketContainer(nlohmann::json &packet) : packet(packet) {};
        int getHead(int &head); 
        int getId(std::string &id);
        virtual int getDataId(std::string &dataId);
        int getData(std::string &data);

		void setHead(int head);
		void setId(std::string id);
		void setTableid(std::string tableId);
		void setPriority(int priority);
		void setData(std::string &data);
		void setStats(nlohmann::json statsBody);
};


/*
	UserdataPacketContainer: This contains the wrapper methods for User data packets.
*/
class UserdataPacketContainer : public PacketContainer {
		nlohmann::json &packet;
	public:
		UserdataPacketContainer(nlohmann::json &packet) : PacketContainer(packet), packet(packet) {};
		int getDataId(std::string &dataId) override;
		int getPriority(int &priority);
		int getTemplateType(std::string &templateType);
		int getBody(std::string &tableId, std::string &instanceType, int &priority, std::string &data);
};

/*
    TemplatePacketContainer: This contains the wrapper methods for Template/Rule packets.
*/
class TemplatePacketContainer : public PacketContainer {
		nlohmann::json &packet;
	public:
		TemplatePacketContainer(nlohmann::json &packet) : PacketContainer(packet), packet(packet) {};
		int getDataId(std::string &dataId) override;
		int getTemplateId(std::string &templateId);
		int getAlgorithmType(int &algoType);
		int getBody(std::string &instanceId, int &algoType, std::string &data);
};

#endif
