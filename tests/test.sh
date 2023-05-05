#!/bin/bash
echo "#----------------------------------------#"
echo "#-------------- Build Test --------------#"
echo "#----------------------------------------#"
echo ""
echo "1. Network test"
echo "2. Data Processing test"
echo "3. Scheduler test"
printf "%b" "Your choice:"
read var

printf "\n%b" "---------------------------------------"
printf "\n%b" "Compiling "

case "$var" in
    "1") printf "%b" "Network";
    printf "\n%b\n\n" "---------------------------------------";
    g++ -I /usr/include/boost unit_test_conn.cpp ../sender_proc/sender.cpp ../sender_proc/forward_stack.cpp ../socket/socket.cpp ../socket/ws_client.cpp ../receiver_proc/receiver.cpp ../sql_access.cpp -g -o conn -pthread -lsqlite3 -std=gnu++17
    ;;
    "2") printf "%b" "Data processor";
    printf "\n%b\n\n" "---------------------------------------";
    g++ unit_test_datproc.cpp ../data_processor.cpp ../algorithm/example.cpp ../algorithm/candidate_elemination.cpp ../sql_access.cpp -g -o dataproc -pthread -lsqlite3 -std=gnu++17
    ;;
    "3") printf "%b" "Scheduler";
    printf "\n%b\n\n" "---------------------------------------";
    g++ unit_test_sched.cpp ../sched.cpp ../thread_pool.cpp ../algorithm/algo.cpp ../algorithm/example.cpp ../algorithm/candidate_elemination.cpp ../sql_access.cpp -g -o sched -pthread -lsqlite3 -std=gnu++17
    ;;
esac

printf "\n%b" "---------------------------------------"
printf "\n%b" "Compilation successful"
printf "\n%b\n" "---------------------------------------"
