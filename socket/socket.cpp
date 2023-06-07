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
    SocketContainer *socketContainer = (SocketContainer*)data;
    //validate the received packets and process them
    DEBUG_MSG(__func__, "normal mode connection: packet: ", socketContainer->packet);
    init_receiver(ws_client_launch(socketContainer->packet));
    wsLock.resetFlag();
    return 0;
}

void *launch_client_socket_QS(void *data)
{
    SocketContainer *socketContainer = (SocketContainer*)data;
    //validate the received packets and process them
    DEBUG_MSG(__func__, "quick send mode connection: packet:", socketContainer->packet);
    init_receiver(ws_client_launch(socketContainer->packet));
    inQSMode.resetFlag();
    Log().info(__func__, "exited quicksend mode");
    return 0;
}

void* socket_task(void *data)
{
    struct socket *soc = (struct socket*)data;
    SocketContainer *container = new SocketContainer(soc);
    SocketContainer *quickModeContainer = new SocketContainer(soc);;
    json packet;
    void *res;

    Log().info(__func__, "socket thread running");
    while(!soc->socketShouldStop)
    {
        //get the latest packet to be sent to the server
        if(!wsLock.isFlagSet())
        {
            wsLock.setFlag(); 
            packet = senderSink.popPacket();
            container->packet = packet;
            //create thread and wait for results
            pthread_create(&wsClientThread, NULL, launch_client_socket, container);
        }
        else if(quickSendMode.isFlagSet() && !inQSMode.isFlagSet())
        {
            inQSMode.setFlag();
            packet = senderSink.popPacket();
            // This thread should not lock in any case
            int head = packet["head"];
            head |= P_QSEND;
            packet["head"] = head;
            quickModeContainer->packet = packet;
            //DEBUG_MSG(__func__, "quick mode packet:",  quickModeContainer->packet.dump());
            // create thread and wait for results
            pthread_create(&wsClientThread, NULL, launch_client_socket_QS, quickModeContainer);
        }
        //sleep(2);
    }
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