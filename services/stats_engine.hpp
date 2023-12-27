#ifndef STATS_H
#define STATS_H
#include <vector>
#include <list>
#include <semaphore.h>
#include <chrono>
#include "../include/task.hpp"
#include "../include/flag.h"
#include "../lib/nlohmann/json-schema.hpp"
#include "../services/global_objects_manager.hpp"

using json = nlohmann::json;

enum TaskStatus {
    TASK_IN,
    TASK_OUT
};

class StatisticsEngine : public Base {
        sem_t taskStatsUpdateLock, timerBeforeLock, timerAfterLock, calcAvgLock;
        int statisticsArraySize;
        int threadCount;
        double averageQueueTime;    // Holds the average value of time vector
        std::vector<int> taskCountVector;   // Gives the current task map in thread queue
        std::vector<int> queueTimeVector;   // Stores average times of thread tasks 
        std::list<std::chrono::high_resolution_clock::time_point> timeBefore;
        std::chrono::high_resolution_clock::time_point timeAfter;
    public:
        StatisticsEngine(){};
        StatisticsEngine(int threadCount, int statisticsArraySize);
        static const std::string& getId()
        {
            static const std::string id = "StatsEngine";
            return id;
        }
		void updateTimerBefore();
        void updateTimerAfter();
        void calculateAvgTimerVector();
        void updateThreadStats(TaskPriority taskPriority, TaskStatus taskStatus);
        json toJson();
};

#endif
