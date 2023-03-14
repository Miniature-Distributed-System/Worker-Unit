#include <pthread.h>
#include <unistd.h>
#include "socket.hpp"
#include "ws_client.hpp"
#include "../receiver_proc/receiver.hpp"
#include "../sender_proc/sender.hpp"
#include "../include/debug_rp.hpp"

pthread_cond_t socket_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
std::string computeID;
pthread_t wsClientThread;
Flag quickSendMode;
Flag seizeMode;
Flag fastSendMode;
Flag wsLock;

struct SocketContainer {
    json packet;
    socket *soc;
    SocketContainer(socket *soc){
        this->soc = soc;
    }
};

void *launch_client_socket(void *data)
{
    SocketContainer *socketContainer = (SocketContainer*)data;
    //validate the received packets and process them
    init_receiver(socketContainer->soc->thread, ws_client_launch(socketContainer->soc, socketContainer->packet));
    if(!quickSendMode.isFlagSet())
        fastSendMode.resetFlag();
    wsLock.resetFlag();
    return 0;
}

void* socket_task(void *data)
{
    struct socket *soc = (struct socket*)data;
    SocketContainer *container = new SocketContainer(soc);
    SocketContainer *quickModeContainer = new SocketContainer(soc);;
    json packet;
    void *res;

    DEBUG_MSG(__func__, "socket thread running");
    while(!soc->socketShouldStop)
    {
        //get the latest packet to be sent to the server
        if(!wsLock.isFlagSet())
        {
            wsLock.setFlag(); 
            packet = getPacket();
            container->packet = packet;
            DEBUG_MSG(__func__, "packet:",  container->packet.dump());
            //create thread and wait for results
            pthread_create(&wsClientThread, NULL, launch_client_socket, container);
        }
        else if(quickSendMode.isFlagSet() && !fastSendMode.isFlagSet())
        {
            fastSendMode.setFlag();
            packet = getPacket();
            quickModeContainer->packet = packet;
            packet["id"] = "quick send";
            DEBUG_MSG(__func__, "quick mode packet:",  quickModeContainer->packet.dump());
            //create thread and wait for results
            pthread_create(&wsClientThread, NULL, launch_client_socket, quickModeContainer);
        }
        sleep(2);
    }
    DEBUG_MSG(__func__, "Shutting down socket");
    return 0;
}

struct socket* init_socket(struct thread_pool *thread, std::string args[])
{
    pthread_t socketThread;
    struct socket* soc = new socket;
    std::string hostname = args[0];
    std::string port = args[1];

    wsLock.initFlag();
    fastSendMode.initFlag();
    quickSendMode.initFlag();
    seizeMode.initFlag();
    soc->thread = thread;
    soc->hostname = hostname;
    soc->port = port;
    soc->socketShouldStop = 0;
    DEBUG_MSG(__func__, "socket initlized");
    pthread_create(&socketThread, NULL, socket_task, soc);

    return soc;
}

void exit_socket(struct socket *soc)
{
    while(fwdStack.getForwardStackSize()){
        sleep(2);
    }
    soc->socketShouldStop = 1;
    pthread_cancel(wsClientThread);
}