#ifndef FWDSTK_H
#define FWDSTK_H

#include <iostream>
#include <semaphore.h>
#include <list>
#include "../include/task.hpp"
#include "../include/packet.hpp"

#define TOTAL_DIFFRED_PACKETS 6

struct ForwardStackPackage {
    SenderDataType statusCode;
    int priority;
    std::string tableID;
    std::string data;

    ForwardStackPackage(){
        statusCode = DAT_NULL;
        priority = DEFAULT_PRIORITY;
        tableID = "";
        data = "";
    }
    
    void copyPointerObject(ForwardStackPackage *item){
        statusCode = item->statusCode;
        priority = item->priority;
        tableID = item->tableID;
        data = item->data;
    }
};

struct AwaitStackPackage {
    struct ForwardStackPackage* fwd_element;
    bool itemAcked;
    AwaitStackPackage(){
        itemAcked = false;
    }
};

class AwaitStack {
    private:
        sem_t stackLock;
        AwaitStackPackage *awaitStack[TOTAL_DIFFRED_PACKETS];
        std::uint8_t index = 0;
        std::uint8_t ackedPackets = 0;
        void purge();
    public:
        AwaitStack(){
            sem_init(&stackLock, 0 ,1);
        }  
        int push(struct ForwardStackPackage*);
        int matchItem(int, std::string);
        bool isFree();
};

class ForwardStack {
    private:
        sem_t stackLock;
        std::list<ForwardStackPackage*> senderStack;
    public:
        AwaitStack awaitStack;
        ForwardStack(){
            sem_init(&stackLock, 0, 1);
        }
        int push(std::string data, std::string tableID, SenderDataType statusCode, int priority);
        int pushFront(std::string data, std::string tableID, SenderDataType statusCode, int priority);
        ForwardStackPackage popForwardStack(void);
        int size(void);
        int isEmpty();
        ForwardStackPackage* getNonackableItem();
};

#endif