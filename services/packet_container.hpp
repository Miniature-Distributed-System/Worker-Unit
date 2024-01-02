#ifndef PKT_CNT_H_
#define PKT_CNT_H
#include "../include/json.hpp"


class PacketContainer {
        nlohmann::json &packet;
    public:
        PacketContainer(nlohmann::json &packet) : packet(packet) {};
        int getHead(int &head); 
        int getId(std::string &id);
        int getTableId(std::string &tableId);
        int getPriority(int &priority);
        int getTemplateType(std::string &templateType);
		int getTemplateId(std::string &templateId);
        int getData(std::string &data);
		int getAlgorithmType(int &algoType);
		int getUserDataBody(std::string &tableId, std::string &instanceType, int &priority, std::string &data);
		int getTemplateBody(std::string &instanceId, int &algoType, std::string &data);

		void setHead(int head);
		void setId(std::string id);
		void setTableid(std::string tableId);
		void setPriority(int priority);
		void setData(std::string &data);
		void setStats(nlohmann::json statsBody);
};

#endif
