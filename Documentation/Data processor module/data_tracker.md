# Data Tracker

This is responsible for tracking the parellel validator phase. it schedules the validation phase. It waits for updates for each of these scheduled 
phases and once it receives all updates it goes to the next stage of the pipeline.

# Working

- The `schedule_validate_phase` method takes `TableData` and `InstanceData` objects.
- It checks the number of threads available for scheduling tasks and it divides the user data in such a way each thread gets one split of user task.
- The validation phase is given start and end index which denotes from which row till which should the validator process.
- The `update_clean_stages` is called by the validators which have completed their tasks.
- This method maintains a counter which keeps track of all validators. If counter reaches the value of number of scheduled validators it takes action.
- The action is it pulls the processed user data and sends to the `Sender` module so the new cleaned user data is sent to the server.
- It calls `sched_algo` to continue the pipeline to the next stage and exits.
