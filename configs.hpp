#ifndef _CONFIG_H
#define _CONFIG_H
#include <string>

class Configs{
        int totalAvailableThreads;
        std::string hostName;
        std::string portNumber;
    public:
        Configs(){}
        Configs(int totalAvailableThreads, std::string hostName, std::string portNumber) : 
            totalAvailableThreads(totalAvailableThreads), hostName(hostName), portNumber(portNumber){}
        int getTotalThreadCount(){ return totalAvailableThreads; }
        std::string getHostName(){ return hostName; }
        std::string getPortNumber(){ return portNumber; }
};

extern Configs globalConfigs;

#endif