#ifndef FORWD_H_
#define FORWD_H_
#include "../include/flag.h"
#include "forward_stack.hpp"
#include "../lib/nlohmann/json-schema.hpp"

using json = nlohmann::json;

class SenderSink{
        ForwardStack fwdStack;
        Flag initSender;
    public:
        SenderSink(){
            initSender.initFlag(false);
        }
        int pushPacket(std::string data, std::string tableID, packet_code statusCode, TaskPriority priority);   
        json popPacket(void);
        int matchItemInAwaitStack(int statusCode, std::string tableID);
        int getForwardStackSize() { return fwdStack.getForwardStackSize(); }
        int isForwardStackEmpty(){ return fwdStack.isForwardStackEmpty(); }
};

extern SenderSink senderSink;
#endif