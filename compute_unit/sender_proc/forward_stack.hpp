#ifndef FWDSTK_H
#define FWDSTK_H

#include <iostream>
#include <semaphore.h>
#include <list>
#include "../include/task.hpp"
#include "../include/packet.hpp"

#define TOTAL_DIFFRED_PACKETS 6

struct fwd_stack_bundle {
    packet_code statusCode;
    int priority;
    std::string tableID;
    std::string data;

    fwd_stack_bundle(){
        statusCode = DAT_NULL;
        priority = DEFAULT_PRIORITY;
        tableID = "";
        data = "";
    }
    
    void copyPointerObject(fwd_stack_bundle *item){
        statusCode = item->statusCode;
        priority = item->priority;
        tableID = item->tableID;
        data = item->data;
    }
};

struct await_stack_bundle {
    struct fwd_stack_bundle* fwd_element;
    bool itemAcked;
    await_stack_bundle(){
        itemAcked = false;
    }
};

class ForwardStack {
    private:
        sem_t stackLock;
        std::list<fwd_stack_bundle*> senderStack;
    public:
        ForwardStack(){
            sem_init(&stackLock, 0, 1);
        }
        int pushToForwardStack(std::string data, std::string tableID, packet_code statusCode, int priority);
        int pushFrontForwardStack(std::string data, std::string tableID, packet_code statusCode, int priority);
        fwd_stack_bundle popForwardStack(void);
        int getForwardStackSize(void);
        int isForwardStackEmpty();
        fwd_stack_bundle* getNonackableItem();
};

class AwaitStack {
    private:
        sem_t stackLock;
        await_stack_bundle *awaitStack[TOTAL_DIFFRED_PACKETS];
        std::uint8_t index = 0;
        std::uint8_t ackedPackets = 0;
        void cleanupAwaitStack();
    public:
        AwaitStack(){
            sem_init(&stackLock, 0 ,1);
        }  
        int pushToAwaitStack(struct fwd_stack_bundle*);
        int matchItemWithAwaitStack(int, std::string);
        bool isAwaitStackFree();
};

extern ForwardStack fwdStack;
extern AwaitStack awaitStack;
#endif