#ifndef SOCKET_H
#define SOCKET_H
#include <iostream>
#include <semaphore.h>
#include "../include/flag.h"

extern pthread_cond_t socket_cond;
extern pthread_t wsClientThread;
extern Flag quickSendMode;
extern Flag seizeMode;

enum SocketStatus {
    SOC_DEFAULT         = 1 << 0,
    SOC_SEIZE           = 1 << 1,
    SOC_SETQS           = 1 << 2,
    SOC_NORMAL_MODE     = 1 << 3,
    SOC_QUICKSEND_MODE  = 1 << 4
};

class Socket {
        Flag quickSendMode;
        Flag seizeMode;
        Flag inQSMode;
        Flag wsLock;
        Flag socketShouldStop;
        sem_t flagLock;
    public:
        Socket();
        void init();
        void exit();
        int getSocketStatus();
        void setFlag(SocketStatus statusFlag);
        void resetFlag(SocketStatus statusFlag);
        bool getSocketStopStatus() { return socketShouldStop.isFlagSet(); }
};

extern Socket globalSocket;

void init_socket();
void exit_socket(struct socket *soc);

#endif