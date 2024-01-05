#include <pthread.h>
#include <unistd.h>
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "../receiver_proc/receiver.hpp"
#include "../sender_proc/sender.hpp"
#include "../scheduler/task_pool.hpp"
#include "../services/global_objects_manager.hpp"
#include "../configs.hpp"
#include "socket.hpp"
#include "ws_client.hpp"

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
    SocType socType;
    JsonContainer(SocType socType){
        this->socType = socType;
    }
};


/* launch_client_socket(): This method is launches the websocket and returns packet
* The function determines which mode the websocket is being launched and based on that perfroms lock reset.
* It launches the websocket by passing the `data to be sent` as argument and waits for `data to be processed` as end
* return value. Once the data is received it schedules the Receiver stage and resets the lock for the appropriate mode
* There are only at most two instances of this Function/thread running at a time.
*/
void *launch_client_socket(void *data)
{
    JsonContainer *jsonContainer = (JsonContainer*)data;
 	std::unique_ptr<nlohmann::json> upPacket = globalObjectsManager.get<SenderSink>().popPacket();
    if(!upPacket){
    	Log().error(__func__, "Failed to fetch packet from sender unit");
        return 0;
    }
	if(jsonContainer->socType == NORMAL){
        Log().info(__func__, "normal mode packet: ", (*upPacket).dump());
    } else {
        Log().info(__func__, "quick mode packet: ", (*upPacket).dump());
    }
    std::unique_ptr<nlohmann::json> packet = ws_client_launch(std::move(upPacket));

	if(packet.get() != nullptr)
    	init_receiver(std::move(packet));
    if(jsonContainer->socType == NORMAL){
        globalObjectsManager.get<Socket>().resetFlag(SOC_NORMAL_MODE);
    } else {
        globalObjectsManager.get<Socket>().resetFlag(SOC_QUICKSEND_MODE);
    }

    return 0;
}

/* socket_task: This thread gets `data to be sent` and lauches the websocket thread`
* Thius pthread rusn for the whole lifetime of the program. It is a iterative thread and is responsible for launching
* the launch_client_socket() function. This method determines the mode in which launch_client_socket() must be laucnged
* Depeneding on the availablity of launch_client_socket() resource it launches in either NORMAL or QUICKSEND mode.
*/
void* socket_task(void *data)
{
    JsonContainer *nModeContainer = new JsonContainer(NORMAL);
    JsonContainer *qModeContainer = new JsonContainer(QUICKSEND);
    pthread_t wsClientThread;

    Socket &socket = globalObjectsManager.get<Socket>();
    Log().info(__func__, "socket thread running...");
    while(!socket.getSocketStopStatus())
    {
		 
        int socStatus = socket.getSocketStatus();
        if(!(socStatus & SOC_NORMAL_MODE))
        {
            
            socket.setFlag(SOC_NORMAL_MODE);
            pthread_create(&wsClientThread, NULL, launch_client_socket, nModeContainer);
        }
        else if(!(socStatus & SOC_QUICKSEND_MODE) && globalObjectsManager.get<SenderSink>().isSenderInitilized())
        {
            socket.setFlag(SOC_QUICKSEND_MODE);
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


/* getSocketStatus(): Resturns the preset websocket parameter status.
 * This method can be used for returning the parameter set status as a single variable. The bitmasks are set for each
 * parameter. Using bitmasks we can determine the status of individual parameters. Its lock coordinated therefore any
 * change happens after returning value only.
*/
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


/* setFlag(): This method is used for setting individual socket parameters.
* This method takes status flag as argument. It sets status of flag to `true` for the passed flag. Only one status can
* be set at a time. This is semaphore lock cooridinated therefore threadsafe.
*/
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

/* resetFlag(): This method resets the flag status of individual socket parameters.
* This method takes status flag as argument. It sets status of flag to `false` for the passed flag. Only one status can
* be reset at a time. This is semaphore lock cooridinated therefore threadsafe. 
*/
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


/* init(): This method starts the socket thread which allows for receving and sending of packets to server.
*/
void Socket::init()
{
    pthread_t socketThread;
    Log().info(__func__, "socket initlized");
    pthread_create(&socketThread, NULL, socket_task, NULL);
}

void Socket::exit()
{
    while(!globalObjectsManager.get<SenderSink>().isEmpty()){
        sleep(2);
    }
}
