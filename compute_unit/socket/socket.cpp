#include <pthread.h>
#include "socket.hpp"
#include "ws_client.hpp"
#include "../receiver_proc/receiver.hpp"
#include "../sender_proc/sender.hpp"
#include "../include/debug_rp.hpp"

pthread_cond_t socket_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
std::string computeID;

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

    cont->soc = soc;
    DEBUG_MSG(__func__, "socket thread running");
    while(!soc->socketShouldStop)
    {
        err = 0;
        //get the latest packet to be sent to the server
        packet = getPacket();
        DEBUG_MSG(__func__, "packet:", packet.dump());
        //send forward port packets and recive incoming packets
        packet = ws_client_launch(soc, packet);
        err = validatePacket(packet) ? -1 : init_receiver(soc->thread, packet);
        if(err){
            DEBUG_ERR(__func__, "pushing error signal to fwd stack");
            send_packet("","", RECV_ERR);
        } else {
            DEBUG_MSG(__func__, "sleeping until  processing is done");
            pthread_cond_wait(&socket_cond, &lock);
        cont->packet = packet;
        DEBUG_MSG(__func__, "packet:",  cont->packet.dump());
        }
    }
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