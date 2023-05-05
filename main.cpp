#include <unistd.h>
#include <cassert>
#include "scheduler/thread_pool.hpp"
#include "scheduler/sched.hpp"
#include "sql_access.hpp"
#include "socket/socket.hpp"
#include "instance/instance_list.hpp"

void menu(){
    std::cout << "\n=========Control Menu==========" << std::endl;
    std::cout << "1.Show Stats" << std::endl;
    std::cout << "2.Exit" << std::endl;
    std::cout << "Choose option:";
}

DatabaseAccess *dataBaseAccess;
DataBaseInstanceList instanceList;

int main()
{
    struct thread_pool *thread;
    struct socket *soc;
    std::string net[2] = {"0.0.0.0", "8080"};
    std::string hostname, port;
    std::uint8_t tNum;
    std::cout << "---------------------------Start Menu--------------------------"<<std::endl;
    std::cout << "Enter Hostname: ";
    std::cin >> hostname;
    std::cout << "Enter Port number: ";
    std::cin >> port;
    if(hostname.empty() || port.empty()){
        std::cout << "\nFalling back to default port and hostname as given is incorrect" << std::endl;
    } else {
        net[0] = hostname;
        net[1] = port;
    }
    std::cout <<"Initing database..." << std::endl;
    assert(sqlite3_config(SQLITE_CONFIG_SERIALIZED) == SQLITE_OK);
    dataBaseAccess = new DatabaseAccess();
    dataBaseAccess->initDatabase();
    std::cout << "Initing task pool..." << std::endl;
    thread = init_thread_pool();
    std::cout << "Enter thread count (max:"<<MAX_THREAD - 1<<"): ";
    std::cin >> tNum;
    tNum = (tNum > MAX_THREAD || tNum <= 0) ? 2 : tNum;
    std::cout << "Initing " << tNum << " Threads..." << std::endl;
    init_sched(thread, tNum);
    std::cout << "Initing and starting sockets..." << std::endl;
    soc = init_socket(thread, net);

    while(1){
        menu();
        int choice;
        std::cin >> choice;
        switch(choice){
            case 1:break;
            case 2:
                std::cout << "Sending seize signal" <<std::endl;
                std::cout << "cleaning up task pool..." <<std::endl;
                exit_thread_pool(thread);
                std::cout << "Shutting down scheduler..." <<std::endl;
                exit_sched();
                std::cout << "Shutting down sockets..." <<std::endl;
                exit_socket(soc);
                exit(0);
            default: break;
        }
    }
    std::cout << "All process shutdown successfully" <<std::endl;
    return 0;
}