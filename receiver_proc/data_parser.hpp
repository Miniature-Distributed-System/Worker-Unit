#ifndef DATA_PARSER_H
#define DATA_PARSER_H

#include <string>
#include "../data_processor/data_processor.hpp"
#include "../services/file_database_access.hpp"
#include "../lib/nlohmann/json-schema.hpp"
using json = nlohmann::json;

//These are local status codes
enum ReceiverStatus {
    P_EMPTY = 0,
    P_OK,
    SND_RESET,
    P_ERROR,
    P_DAT_ERR,
    P_SUCCESS,
    P_VALID,
};

class InstanceDataParser {
        std::string insertValueQuery;
        std::string createTableQuery;
        std::string verifyTableQuery;
        std::string dropTableQuery;
        std::string *colHeaders;
        int columns;
        int rows;
        std::string tableId;
        json packet;
    public:
        InstanceDataParser(json packet) : packet(packet) {};
        int createSqlCmds(int cols, std::string body);
        int insertIntoTable(std::string data, int startIndex);
        void dropTable();
        void constructInstanceObjects(std::uint8_t algoType);
        ReceiverStatus processInstancePacket(std::string &tableId);
};

class UserDataParser {
        std::string insertValueQuery;
        std::string createTableQuery;
        std::string verifyTableQuery;
        std::string dropTableQuery;
        std::string tableId;
        int columns;
        int rows;
        json packet;
        std::string *colHeaders;
        DataProcessContainer *dataProcContainer;
        FileDataBaseAccess *fileDataBaseAccess;
    public:
        UserDataParser(json packet) : packet(packet) {};
        int createSqlCmds(int cols, std::string body);
        int insertIntoTable(std::string data, int startIndex);
        void dropTable();
        void constructDataObjects(std::uint8_t priority, std::string algoType);
        ReceiverStatus processDataPacket(std::string &tableId, DataProcessContainer *dataProcessContainer);
};

#endif