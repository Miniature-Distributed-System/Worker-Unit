#include <unistd.h>
#include <cassert>
#include "algorithm/algorithm_scheduler.hpp"
#include "scheduler/task_pool.hpp"
#include "scheduler/sched.hpp"
#include "socket/socket.hpp"
#include "sender_proc/sender.hpp"
#include "instance/instance_list.hpp"
#include "services/stats_engine.hpp"
#include "services/sqlite_database_access.hpp"
#include "include/logger.hpp"
#include "configs.hpp"

void menu(){
    std::cout << "\n=========Control Menu==========" << std::endl;
    std::cout << "1.Show Stats" << std::endl;
    std::cout << "2.Exit" << std::endl;
    std::cout << "Choose option:";
}

SqliteDatabaseAccess *sqliteDatabaseAccess;
DataBaseInstanceList instanceList;
StatisticsEngine statsEngine;
Configs globalConfigs;
SenderSink senderSink;
TaskPool taskPool;
Socket globalSocket;
std::map<std::string, AlgorithmPackage *> algorithmResultMap;

int main()
{
    struct ThreadPool *thread;
    std::string net[2] = {"0.0.0.0", "8080"};
    std::string hostname, port;
    int threadCount;
    std::cout << "\033[1;97;49m---------------------------Start Menu--------------------------\033[0m" << std::endl;
    std::cout << "\033[1;33;49mEnter Hostname: \033[0m";
    std::cin >> hostname;
    std::cout << "\033[1;33;49mEnter Port number: \033[0m";
    std::cin >> port;
    if(hostname.empty() || port.empty()){
        std::cout << "\nFalling back to default port and hostname as given is incorrect" << std::endl;
    } else {
        net[0] = hostname;
        net[1] = port;
    }
    assert(sqlite3_config(SQLITE_CONFIG_SERIALIZED) == SQLITE_OK);
    sqliteDatabaseAccess = new SqliteDatabaseAccess();
    sqliteDatabaseAccess->initDatabase();
    std::cout <<"Inited database" << std::endl;

    std::cout << "Inited task pool" << std::endl;

    std::cout << "\033[1;33;49mEnter thread count (max:"<<MAX_THREAD - 1<<"): \033[0m";
    std::cin >> threadCount;
    
    globalConfigs = Configs(threadCount, hostname, port);

    threadCount = (threadCount > MAX_THREAD || threadCount <= 0) ? 2 : threadCount;
    init_sched(threadCount);
    std::cout << "Inited " << threadCount << " Threads" << std::endl;

    statsEngine = StatisticsEngine(threadCount, 10);
    std::cout << "Inited Stats Engine" << std::endl;

    globalSocket.init();
    std::cout << "Inited and running sockets" << std::endl;

    std::cout << "\033[1;97;49m---------------------------DEBUGGER START--------------------------\033[0m" << std::endl;

    while(1){
        // menu();
        // int choice;
        // std::cin >> choice;
        // switch(choice){
        //     case 1:break;
        //     case 2:
        //         std::cout << "Sending seize signal" <<std::endl;
        //         std::cout << "cleaning up task pool..." <<std::endl;
        //         exit_thread_pool(thread);
        //         std::cout << "Shutting down scheduler..." <<std::endl;
        //         exit_sched();
        //         std::cout << "Shutting down sockets..." <<std::endl;
        //         exit_socket(soc);
        //         exit(0);
        //     default: break;
        // }
    }
    std::cout << "All process shutdown successfully" <<std::endl;
    return 0;
}