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
bool quickSendMode = false;
bool seizeMode = false;

struct socket_container {
    json packet;
    struct socket *soc;
};

void *launch_client_socket(void *data)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    struct socket_container* cont = (struct socket_container*)data;
    cont->packet = ws_client_launch(cont->soc, cont->packet);

    return 0;
}

void* socket_task(void *data)
{
    struct socket *soc = (struct socket*)data;
    struct socket_container *cont = new socket_container;
    json packet;
    int err;
    void *res;

    cont->soc = soc;
    DEBUG_MSG(__func__, "socket thread running");
    while(!soc->socketShouldStop)
    {
        err = 0;
        //get the latest packet to be sent to the server
        packet = getPacket();
        cont->packet = packet;
        DEBUG_MSG(__func__, "packet:",  cont->packet.dump());
        //create thread and wait for results
        pthread_create(&wsClientThread, NULL, launch_client_socket, (void*)cont);
        pthread_join(wsClientThread, &res);
        if(res == PTHREAD_CANCELED){
            DEBUG_MSG(__func__, "sender cancelled the socket thread");
            continue;
        }
        //validate the received packets and process them
        init_receiver(cont->soc->thread, cont->packet);
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
    soc->socketShouldStop = 1;
    delete soc;
}