# Statistics Engine

This module polls the workers and calculates the worker load and task queue load type. 
This module provides these methods.

## Average Queuing Time

### updateTimerBefore()

- This method is used by the task pool when a new task is added to pool.
- When this task is called the `timeBefore` records the time in the queue at the time of calling the method.

### updateTimerAfter()

- This method is used by the task pool when the top task is popped out.
- The `queueTimeVector` is pushed with the value of `timeAfter` - `timeBefore`.
- When the `queueTimeVector` is full the `calculateAvgTimerVector`.

### calculateAvgTimerVector()

- This method is called by the `updateTimerAfter` when the `queueTimeVector` is full.
- It aggrigates all the times in the vector and calculates the average.
- The `StatisticsEngine` variable `averageQueueTime` is updated which is sent out as stats by Sender.

## Priority tasks

### updateThreadStats()

- This method is called by Task pool when either a new task is added or popped out.
- The Method can be invoked with `TaskStatus` which can be:-
    - `TASK_IN`
    - `TASK_OUT`
- The task when added is called with `TASK_IN` argument and the task priority is added to the `taskCountVector` vector.
- The task when popped the method is called with `TASK_OUT` argument and the related task priority is removed.
- This data is used by Sender module to give Server more information about worker unit current work load and aid its decesion
making.

## To Json

- `toJson` converts the `StatisticsEngine` objects data to json format which can be readily pushed into packet body.
- It has `stats` as the main body and under it it stores:
  - `taskQueue` which holds number of priority tasks vector as string
  - `avgQueueTime` which holds Averge queuing time
  - `totalAvailableThreads` which holds the total schedulable threads available for worker process
