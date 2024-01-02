#include "stats_engine.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"

StatisticsEngine::StatisticsEngine(int threadCount, int statisticsArraySize) : threadCount(threadCount),
    statisticsArraySize(statisticsArraySize)
{
    sem_init(&calcAvgLock, 0, 1);
    sem_init(&timerAfterLock, 0, 1);
    sem_init(&timerBeforeLock, 0, 1);
    sem_init(&taskStatsUpdateLock, 0 ,1);
    for(int i = 0; i <= NR_TASK_PRIORITY; i++)
        taskCountVector.push_back(0);
    averageQueueTime = 0;
    Log().info(__func__, "inited stats engine");
}

/* calculateAvgTimerVector(): Takes the whole of queueTimeVector and averages all its values and updates 
 * averageQueueTime member variable.
 * Desc: This method gives us the average of the all times recorded for the updateTimerBefore() and updateTimerAfter()
 * This gives us an idea of how much the worker is capable of taking in tasks. It tells us if worker is getting
 * overwhelemed with tasks and the server needs to hold off tasks for a while for worker to finish the pending tasks.
*/
void StatisticsEngine::calculateAvgTimerVector()
{
    int totalSize = queueTimeVector.size();
    double timerAggrigate = 0;
    double finalAverage;

    if(queueTimeVector.size() == 0)
        return;

    sem_wait(&calcAvgLock);
    for(auto i = queueTimeVector.begin(); i != queueTimeVector.end(); i++){
        timerAggrigate += *i;
    }
    finalAverage = timerAggrigate / totalSize;
    queueTimeVector.clear();
    queueTimeVector.push_back(finalAverage);
    averageQueueTime = finalAverage;
    sem_post(&calcAvgLock);
}

/* updateTimerBefore(): Updates the before timer when task is pushed into the thread pool
*/
void StatisticsEngine::updateTimerBefore()
{
    sem_wait(&timerBeforeLock);
    timeBefore.push_back(std::chrono::high_resolution_clock::now());
    sem_post(&timerBeforeLock);
}

/* updateTimerAfter(): Updates the after timer when task is popped from thread poo and calculates the diffrence of 
*  after and before timer
*  Desc: The values from above method are used and this method calculates the time taken by the thread between pushing
*  new element in and popping another element. It essentaally gives stats of frequently a item gets in/out longer means
*  the worker unit has more task or is struggling with tasks due to thread limits or processor performance.
*/
void StatisticsEngine::updateTimerAfter()
{
    sem_wait(&timerAfterLock);
    timeAfter = std::chrono::high_resolution_clock::now();
    queueTimeVector.push_back(std::chrono::duration_cast<std::chrono::milliseconds>
        (timeAfter - timeBefore.front()).count());
    sem_wait(&timerBeforeLock);
    if(!timeBefore.size())
        timeBefore.pop_front();
    sem_post(&timerBeforeLock);
    if(queueTimeVector.size() >= statisticsArraySize)
            calculateAvgTimerVector();
    sem_post(&timerAfterLock);
}

/* updateThreadStats(): Increment the counter for the repective task type
 * Desc: This method updates the counter for each task type which is divided based on priority. It counts how many 
 * tasks of a certain priority exists in the task pool currently.
*/
void StatisticsEngine::updateThreadStats(TaskPriority taskPriority, TaskStatus taskStatus)
{
    sem_wait(&taskStatsUpdateLock);
    if(taskStatus == TASK_IN){
         taskCountVector[taskPriority]++;
    } else {
        if(taskCountVector[taskPriority])
            taskCountVector[taskPriority]--;
    }
    sem_post(&taskStatsUpdateLock);
}

/* toJson(); Converts the above values recorded to json to be exported to server
*/
nlohmann::json StatisticsEngine::toJson()
{
    nlohmann::json subPacket;
    std::string thread;
    
    // Get current task queue map
    subPacket["stats"];
    thread = std::to_string(taskCountVector[0]);
    for(int i = 1;i < taskCountVector.size(); i++)
        thread += "," + std::to_string(taskCountVector[i]);
    subPacket["stats"]["taskQueue"] = thread;

    // Get the average queue milliseconds
    if(!averageQueueTime)
        calculateAvgTimerVector();
    subPacket["stats"]["avgQueueTime"] = averageQueueTime;

    subPacket["stats"]["totalAvailableThreads"] = threadCount;

    return subPacket["stats"];
}
