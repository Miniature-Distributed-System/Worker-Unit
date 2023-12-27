#include <cstdlib>
#include <unistd.h>
#include <cassert>
#include "algorithm/algorithm_scheduler.hpp"
#include "data_processor/data_tracker.hpp"
#include "scheduler/task_pool.hpp"
#include "scheduler/sched.hpp"
#include "scheduler/process_manager.hpp"
#include "socket/socket.hpp"
#include "sender_proc/sender.hpp"
#include "instance/instance_list.hpp"
#include "services/stats_engine.hpp"
#include "services/sqlite_database_access.hpp"
#include "services/global_objects_manager.hpp"
#include "include/logger.hpp"
#include "configs.hpp"


SqliteDatabaseAccess *sqliteDatabaseAccess;
DataBaseInstanceList instanceList;
StatisticsEngine statsEngine;
Configs globalConfigs;
SenderSink senderSink;
TaskPool taskPool;
Socket globalSocket;
std::map<std::string, AlgorithmPackage *> algorithmResultMap;
std::map<TableData *, int> cleanStageMap;
ProcessManager globalProcessManager;

GlobalObjectsManager globalObjectsManager;
std::string Base::classID = "Base";

int init_modules()
{	
	globalObjectsManager.add(senderSink);
	globalObjectsManager.add(taskPool);
	globalObjectsManager.add(globalSocket);
	globalObjectsManager.add(globalProcessManager);
	
	return 0;
}

int main(int argc, char *argv[])
{	
	std::string hostname, port;
    int threadCount;
	std::string net[2] = {"0.0.0.0", "8080"};
	
	if(argc == 4)
	{
		hostname = argv[1];
		port = argv[2];
		threadCount = std::atoi(argv[3]); 
	} else {
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
		 std::cout << "\033[1;33;49mEnter thread count (max:"<<MAX_THREAD - 1<<"): \033[0m";
    	std::cin >> threadCount;
	}

    assert(sqlite3_config(SQLITE_CONFIG_SERIALIZED) == SQLITE_OK);
    sqliteDatabaseAccess = new SqliteDatabaseAccess();
    sqliteDatabaseAccess->initDatabase();
    std::cout <<"Inited database" << std::endl;
	
	init_modules();

    std::cout << "Inited task pool" << std::endl;
    
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
       
    }
    std::cout << "All process shutdown successfully" <<std::endl;
    return 0;
}
