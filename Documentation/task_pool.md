# Task Pool

- Task pools are queues which stores the tasks/pipeline process that needs be executed by the threads.
- The Tasks are ordered based on their priority.
- The priority defination is given in `task.hpp` inside the enum `TaskPriority`.

### TaskPriority

- Each task can take any 4 priority levels.
- The `NON_PREEMTABLE` has the highest priority and `LOW_PRIORITY` being the lowest priority.
- The default is `LOW_PRIORITY`.
- `NR_TASK_PRIORITY` Gives the total number of priority levels available.
  
```
  NON_PREEMTABLE
  HIGH_PRIORITY
  MEDIUM_PRIORITY
  LOW_PRIORITY
```

## Working of task pool

Below given is the basic flow of task pool
```
  1. start
  2. schedule task
  3. register process
  4. push task to queue
  5. loggers
  6. signal scheduler to wake up
  7. exit
```
- The tasks are sceheduled by calling `scheduleTask` method and it accepts `ProcessStates`, argument bundle and `TaskPriority`.
- These three are mandatory arguments and cannot be null, if found null the task is not pushed to pool and returns signal `EXIT_FAILURE`.
- If all are validated and as per requirements then the passed data is sent to process manager calling `registerProcess` method which outputs the
`TaskData` structure as result.
- This received `TaskData` needs to be pushed to waiting queue by calling `pushTask` method.
- The scheduler thread is woken up after insertion of task and exits.
  
### Inserting Task

- The `pushTask` holds the queue lock so no other parellel process accesses the queue and cause potential queue corruption.
- The `pushTask` goes through the queue and compares the priority of the task on hand with task in queue.
- The method iterates through the queue comparing each task from queue with their priority.
- If the task on hand has higher priority than task in queue the task on hand is inserted before the lower priority task.
- To prevent task starvation we have a starve counter for every task in the queue.
- The starve counter is incremented everytime the task gets pushed back by a higher priority task.
- When the task is starved then the higher priority task skips to the next task to see if it can be inserted there or not.
- If task is lower priority than all tasks in queue its simply pushed to end of queue.
- Before exiting the queue lock is released.

### Popping task

- The scheduler pulls the top most task from the queue using `getScheduledTask` method passing the `TaskData` structure as reference.
- The `popTask` method is called.
- The task queue is checked if it has any pending tasks if none then passes signal `EXIT_FAILURE`.
- `popTask` holds queue lock.
- It pulls the front element of queue and erases it from queue.
- the queue lock is released and returns with this refrence to `TaskData` structure it popped.
- with this the task pool process finally exits.
