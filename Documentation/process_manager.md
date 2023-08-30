# Process Manager

Process manager creates a process table for every submitted task. Each process table has a `ProcessTable` structure. Each `ProcessTable` is pushed and stored in process manager stack.

### Process table

- `ProcessTable` defination can be found in  `include/process.hpp`
- Process table stores the task process id(`pid`) which is of type `uint64`.
- `processState` stores the `ProcessState` and corrosponding arguments.
- The `jobStatus` which determines the status of job executed or not.
- We can set priority of task in `priority` variable of type `TaskPriority`.
- The Process Table deallocates process state when the process table is destroyed.

## Registering process

- The process are registered using the `registerProcess` method which takes `ProcessStates`, arguments bundle and priority of task.
- The method iterates through the loop and generates a unique pid for the task.
- The pid is set for the task and the `ProcessTable` is initilised for the task to be registered.
- The `ProcessTable` once initilised is pushed into a vector.
- The method returns the `ProcessTable` the method created for the task.

## Un-Registering process

- The process is unregistered by the scheduler when the task completes or fails.
- The `unregisterProcess` method is called by passing the process table pointer as argument.
- The passed process table is searched in the vector and erased from the vector.
- The memory is held is released for that process and returns.
