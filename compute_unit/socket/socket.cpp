#include <pthread.h>
#include <unistd.h>
#include "socket.hpp"
#include "ws_client.hpp"
#include "../receiver_proc/receiver.hpp"
#include "../sender_proc/sender.hpp"
#include "../include/debug_rp.hpp"
#include "../include/flag.h"

pthread_cond_t socket_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
std::string computeID;
pthread_t wsClientThread;
bool quickSendMode = false;
bool seizeMode = false;
Flag fastSendMode;
Flag wsLock;

struct socket_container {
    json packet;
    struct socket *soc;
    socket_container *copyObject(){
        socket_container *soc = new socket_container();
        soc->soc = this->soc;
        return soc;
    }
};

void *launch_client_socket(void *data)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    struct socket_container* cont = (struct socket_container*)data;
    cont->packet = ws_client_launch(cont->soc, cont->packet);

    wsLock.resetFlag();
    return 0;
}

void* socket_task(void *data)
{
    struct socket *soc = (struct socket*)data;
    struct socket_container *cont = new socket_container;
    json packet;
    int err;
    void *res;
    bool fastSendMode = false;

    cont->soc = soc;
    DEBUG_MSG(__func__, "socket thread running");
    while(!soc->socketShouldStop)
    {
        err = 0;
        //get the latest packet to be sent to the server
        if(!sem_trywait(&wsClientThreadLock)){
        if(!wsLock.checkFlag())
        {
            wsLock.setFlag(); 
            packet = getPacket();
            cont->packet = packet;
            DEBUG_MSG(__func__, "packet:",  cont->packet.dump());
            //create thread and wait for results
            pthread_create(&wsClientThread, NULL, launch_client_socket, (void*)cont);

            //validate the received packets and process them
            init_receiver(cont->soc->thread, cont->packet);
            sem_post(&wsClientThreadLock);
            if(!quickSendMode)
                fastSendMode = false;
            socket_container *soc = cont->copyObject();
        else if(quickSendMode && !fastSendMode.checkFlag())
        {
            fastSendMode.setFlag();
            packet = getPacket();
            soc->packet = packet;
            DEBUG_MSG(__func__, "packet:",  soc->packet.dump());
            //create thread and wait for results
            pthread_create(&wsClientThread, NULL, launch_client_socket, (void*)soc);

            //validate the received packets and process them
            init_receiver(soc->soc->thread, soc->packet);
            delete soc;
        }
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

    sem_init(&wsClientThreadLock, 0,0);
    wsLock.initFlag();
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
    while(fwdStack.get_fwdstack_size()){
        sleep(2);
    }
    soc->socketShouldStop = 1;
    pthread_cancel(wsClientThread);
}