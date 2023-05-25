#ifndef _CONFIG_H
#define _CONFIG_H
#include <string>

class Configs{
        int totalAvailableThreads;
        std::string hostName;
        std::string portNumber;
        std::string workerId;
    public:
        Configs(){}
        Configs(int totalAvailableThreads, std::string hostName, std::string portNumber) : 
            totalAvailableThreads(totalAvailableThreads), hostName(hostName), portNumber(portNumber){}
        int getTotalThreadCount(){ return totalAvailableThreads; }
        std::string getHostName(){ return hostName; }
        std::string getPortNumber(){ return portNumber; }
        std::string getWorkerId() { return workerId; }
        void setWorkerId(std::string id) { workerId = id; }
};

extern Configs globalConfigs;

#endif