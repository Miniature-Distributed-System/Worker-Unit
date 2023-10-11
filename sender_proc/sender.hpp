#ifndef FORWD_H_
#define FORWD_H_
#include "../include/flag.h"
#include "forward_stack.hpp"
#include "../lib/nlohmann/json-schema.hpp"

using json = nlohmann::json;

class SenderSink{
        ForwardStack fwdStack;
        Flag initSender;
        Flag handshakeSent;
    public:
        SenderSink(){
            initSender.initFlag(false);
            handshakeSent.initFlag(false);
        }
        int pushPacket(std::string data, std::string tableID, SenderDataType statusCode, TaskPriority priority);   
        json popPacket(void);
        int matchItemInAwaitStack(int statusCode, std::string tableID);
        int size() { return fwdStack.size(); }
        int isEmpty(){ return fwdStack.isEmpty(); }
        bool isValidPacket(json packet);
        bool isSenderInitilized() { return initSender.isFlagSet(); }
};

extern SenderSink senderSink;
#endif