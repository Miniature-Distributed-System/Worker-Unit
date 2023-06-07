#ifndef SOCKET_H
#define SOCKET_H
#include <iostream>
#include "../include/flag.h"

extern pthread_cond_t socket_cond;
extern pthread_t wsClientThread;
extern Flag quickSendMode;
extern Flag seizeMode;

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
        int getSocketStatus();
        void setFlag(SocketStatus statusFlag);
        void resetFlag(SocketStatus statusFlag);
        bool getSocketStopStatus() { return socketShouldStop.isFlagSet(); }
};

struct socket* init_socket();
void exit_socket(struct socket *soc);

#endif