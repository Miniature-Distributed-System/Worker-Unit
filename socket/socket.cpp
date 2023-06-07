#include <pthread.h>
#include <unistd.h>
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../receiver_proc/receiver.hpp"
#include "../sender_proc/sender.hpp"
#include "../scheduler/task_pool.hpp"
#include "../configs.hpp"
#include "socket.hpp"
#include "ws_client.hpp"

pthread_cond_t socket_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t wsClientThread;
Flag quickSendMode;
Flag seizeMode;
Flag inQSMode;
Flag wsLock;

enum SocType {
    NORMAL,
    QUICKSEND
};

struct socket {
    std::string hostname;
    std::string port;
    bool socketShouldStop;
};

struct JsonContainer {
    json packet;
    SocType socType;
    JsonContainer(SocType socType){
        this->socType = socType;
    }
};

void *launch_client_socket(void *data)
{
    JsonContainer *jsonContainer = (JsonContainer*)data;
    if(jsonContainer->socType == NORMAL){
        Log().info(__func__, "normal mode packet: ", jsonContainer->packet.dump());
    } else {
        Log().info(__func__, "quick mode packet: ", jsonContainer->packet.dump());
    }
    json packet = ws_client_launch(jsonContainer->packet);
    init_receiver(packet);
    if(jsonContainer->socType == NORMAL){
        globalSocket.resetFlag(SOC_NORMAL_MODE);
    } else {
        globalSocket.resetFlag(SOC_QUICKSEND_MODE);
}

    return 0;
}


void* socket_task(void *data)
{
    JsonContainer *nModeContainer = new JsonContainer(NORMAL);
    JsonContainer *qModeContainer = new JsonContainer(QUICKSEND);
    pthread_t wsClientThread;

    Log().info(__func__, "socket thread running...");
    while(!globalSocket.getSocketStopStatus())
    {
        int socStatus = globalSocket.getSocketStatus();
        if(!(socStatus & SOC_NORMAL_MODE))
        {
            
            globalSocket.setFlag(SOC_NORMAL_MODE);
            nModeContainer->packet = senderSink.popPacket();
            pthread_create(&wsClientThread, NULL, launch_client_socket, nModeContainer);
        }
        else if((socStatus & SOC_SETQS) && !(socStatus & SOC_QUICKSEND_MODE))
        {
            globalSocket.setFlag(SOC_QUICKSEND_MODE);
            json packet = senderSink.popPacket();
            //TO-DO: This code should be moved to sender.cpp
            int head = packet["head"];
            head |= P_QSEND;
            packet["head"] = head;
            qModeContainer->packet = packet;
            pthread_create(&wsClientThread, NULL, launch_client_socket, qModeContainer);
        }
    }

    delete nModeContainer;
    delete qModeContainer;
    Log().info(__func__, "Shutting down socket");

    return 0;
}

Socket::Socket()
{
    quickSendMode.initFlag(false);
    seizeMode.initFlag(false);
    inQSMode.initFlag(false);
    wsLock.initFlag(false);
    socketShouldStop.initFlag(false);
    sem_init(&flagLock, 0 ,1);
}

int Socket::getSocketStatus()
{
    int status = SOC_DEFAULT;
    if(seizeMode.isFlagSet())
        status |= SOC_SEIZE;
    if(quickSendMode.isFlagSet())
        status |= SOC_SETQS;
    if(wsLock.isFlagSet())
        status |= SOC_NORMAL_MODE;
    if(inQSMode.isFlagSet())
        status |= SOC_QUICKSEND_MODE;

    return status;
}

void Socket::setFlag(SocketStatus statusFlag)
{
    sem_wait(&flagLock);
    switch(statusFlag){
        case SOC_SEIZE: seizeMode.setFlag();break;
        case SOC_SETQS: quickSendMode.setFlag();break;
        case SOC_NORMAL_MODE: wsLock.setFlag();break;
        case SOC_QUICKSEND_MODE: inQSMode.setFlag();break;
        default:Log().debug(__func__, "setting unknown flag");break;
    }
    sem_post(&flagLock);
}

void Socket::resetFlag(SocketStatus statusFlag)
{
    sem_wait(&flagLock);
    switch(statusFlag){
        case SOC_SEIZE: seizeMode.resetFlag();break;
        case SOC_SETQS: quickSendMode.resetFlag();break;
        case SOC_NORMAL_MODE: wsLock.resetFlag();break;
        case SOC_QUICKSEND_MODE: inQSMode.resetFlag();break;
        default:Log().debug(__func__, "re-setting unknown flag");break;
    }
    sem_post(&flagLock);
}
{
    pthread_t socketThread;
    struct socket* soc = new socket;

    wsLock.initFlag();
    inQSMode.initFlag();
    quickSendMode.initFlag();
    seizeMode.initFlag();
    soc->hostname = globalConfigs.getHostName();
    soc->port = globalConfigs.getPortNumber();
    soc->socketShouldStop = 0;
    Log().info(__func__, "socket initlized");
    pthread_create(&socketThread, NULL, socket_task, soc);

    return soc;
}

void exit_socket(struct socket *soc)
{
    while(!senderSink.isForwardStackEmpty()){
        sleep(2);
    }
    soc->socketShouldStop = 1;
    pthread_cancel(wsClientThread);
}