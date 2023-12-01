# Scheduler

Task scheduler is responsible for managing all the units execution on the worker unit. The pipeline units are all scheduled and are executed on top of threads made available by
the scheduler. The order of execution, re-scheduling, task error management are all the job of the scheduler.

- When the worker is fired the user is asked to provide the thread count the worker can use (Default is 2). This is passed as argument to the `init_sched` which initilizes the
the allowed number of task threads and a scheduler thread for managing these task threads and monitoring the Task Pool.

## Scheduler Thread

- Any new tasks the Scheduler thread aka `sched_task` will pop tasks and make a job out of the popped task. These Jobs are put inside `QueueJob` Data Structure.
- Each thread has a Queue or `ThreadQueue` data structure that keeps all thread infomation and Jobs in its LILO queue. The Queue/Job Queue has slots and each slot is occupied
by a single Job. Initially the slots are empty and are all blanks. The `getTotalJobsInQueue` will report as 0.
- The scheduler calls the `get_total_empty_slots` method which loops through all Task Threads and for each thread it gets the `getTotalJobsInQueue` and subtracts it with
`QUEUE_SIZE` which gives us empty slots available in that particular Task Thread. This method adds up all the Task Threads empty slots and returns it to the scheduler.
- If the returned empty slots are 0 then scheduler goes back to sleep else it proceeds and checks the Task Pool for pending tasks. if there are queued tasks in the Task Pool
then the top most task is popped out and scheduler creates a `QueueJob` from this task using `init_job`.
- The returned slots are the total amount of jobs that Task Threads Aggrigate can hold in its queue for the time-being.
- The Scheduler then executes `get_quickest_queue` method which finds the fastest thread. This method takes the queue time of each thread. Each `QueueJob` has time slot in
other words the `QueueJob` must finish its work within this time limit else its re-queued on the `ThreadQueue`. it gets the queue having the shortest jobs and at least one
empty slot.
- If there are more than one slot available it will check Task Pool if it has more tasks queued then it will pop them fill all the slots.
- The Jobs that are done are flushed from thread.
- Finally once all jobs are filled it executes the `rebalance_thread_queues` function. This function goes through all threads and finds idle threads and moves jobs from busy
threads to idle threads. It balances all threads so all threads have equal jobs.
- Then scheduler thread goes to sleep only to be woken up by Task Thread or Task Pool.

## Task Thread

- The task thread executes all jobs in its Queue which is a part of `ThreadQueue`.
- The task thread is inited and started by the `init_sched`.
- The task threads keep executing as long as `shouldStop` is false. This becomes true when the worker is signaled to stop.
- It checks its queue initially and if it finds that its not initilized it exits the thread.
- If queue is initilized then the task thread goes through its queue slots using method `getNextTask` which fetches the next'
task. The retrived job is NULL if its empty. It cycles through list until it finds a job.
- Once it finds a job it checks the Job status. Job can be:-
   - `JOB_DONE`
   - `JOB_RUNNING`
   - `JOB_PENDING`
- If job is pending it sets the timer for Job and sets its status as `JOB_RUNNING`.
- It executes the `runStartProcess` job's start process and loops until the timer runs out or until the Job moves to finished phase.
- If the Job timer runs out before job finishes the state is reverted to `JOB_PENDING` after calling the `runPauseProcess()` method to save progress of job and then its re-queued.
- If the Job fails then the task thread sets the `jobErrorHandle.setFlag()` and finally breaks out of the loop.
- If Job succeeds then the job status is set to `JOB_DONE`.
- On the next cycle when the thread comes accross this slot thats set to `JOB_DONE` it runs the `runFailProcess` if the
job failed and `runEndProcess`.
- Then it would unregister the Job from Process Manager and marks job as complete.
- The scheduler thread is signaled to wake up and the jobs marked as complete are flushed by the scheduler.

Thread Queues Working:
```
  Allocated Time Slots based on task Priority by Scheduler
  Job A : 10ms 
  Job B: 12ms
  Job C: 20ms

  Actual Time required to finish jobs
  Job A : 40ms 
  Job B: 12ms
  Job C: 10ms
  

  Initial Queue
  Job Pointer ->| A | B | C |

  After A Job's timer expires
  Job Pointer ->| B | C | A |

  After B Job's timer expires
  Job Pointer ->| C | A | B(Job Done) |

  After C Job exists early timer is halted
  Job Pointer ->| A | (Job flushed by sched) | C(Job Done) |

  After Cycling A 3 More times
  Job Pointer ->| A(Done) | (Slot empty) | (Slot empty) |
  Job Pointer ->| (Job flushed by sched) | (Slot empty) | (Slot empty) |
```
