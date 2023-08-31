# Statistics Engine(WIP)

This engine is used for calculating various worker statistics which can prove to be useful for the server in making decisions during data forwarding.
It helps the server balance the load more appropriatly. the stats are calculated based on task pool fill status and time between when a task enters and
leaves the task pool.

## Working

### Average Queuing Time

- The `updateTimerBefore` and `updateTimerAfter` are used for getting average time of task entering and exiting the task pool.
- `updateTimerBefore` method is called when the task is added to task pool, the entry time is recorded and pushed into `timeBefore` list.
- `updateTimerAfter` method is called when the task is popped from the task pool, the exit time is taken and subtracted with the corrosponding task's exit time.
- These times are pushed into the `queueTimeVector`.
- If the `queueTimeVector` is found be exceding the maximum stats array size then the `calculateAvgTimerVector` is called.
- `calculateAvgTimerVector` iterates through the vector and averages the times recorded and stores it `averageQueueTime` variable of `StatisticsEngine`.
- Each task's queuing times are tracked this way.

### Priority tasks

- `updateThreadStats` is used to update the task pool tasks types.
- The `TaskStatus` enum passed as argument, `TASK_IN` and `TASK_OUT` is used to determine if task is entered or exited from pool
- When a task of certain priority is added to task pool this method is called to keep track of priority task count.
- It keeps track of all available priorties in a vector.
- When task exists the task pool this information is updated by decrimenting count.
- This information is allows server to know how many tasks of certain priority is present in a given worker and make decision accordingly.

### To Json

- `toJson` converts the `StatisticsEngine` objects data to json format which can be readily pushed into packet body.
- It has `stats` as the main body and under it it stores:
  - `taskQueue` which holds number of priority tasks vector as string
  - `avgQueueTime` which holds Averge queuing time
  - `totalAvailableThreads` which holds the total schedulable threads available for worker process
