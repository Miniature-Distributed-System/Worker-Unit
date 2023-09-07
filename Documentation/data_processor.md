# Data Processor

This module is responsible for processing user data. The user data is processed before passing ot the compute stage. This stage has two phases.
The Data Cleaner phase which is serial phase and Data Validator which is a parellel phase.
The Data Processor schedules cleaner phase which in turn schedules validator phase after complition.
The Data tracker keeps track of parellel phases.

# Working

- The data processor is scheduled by the receiver stage.
- The data processor starts by initlizing the `DataProcessor` object using the `initilize` method.
- The `initlize` fetches the `Instance` object(User data rules for processing) corrosponding to the User data in hand, if not found returns error.
- Then it creates a new `InstanceData` object using the fetched `Instance` object and `TableData` object.
- The `InstanceData` is initlized calling the method `initlizeData` method of `InstanceData` class, if any error is encountered the error is returned.
- The `FileDataBaseAccess` is initilized with User Data file name as file to read, the file is opened in `RW_FILE` mode which is read/write mode.
- The column headers names list is retreived from the `FileDataBaseAccess` object.
- initlization is confirmed as done at this point.
- Once the `initlize` method returns successfully `start` of scheduled process is complete and procceds to `finalize`.
- In `finalize` the data cleaner phase is called and the `DataProcessor` object is deleted.

# Error handling

- If in case the `initlize` method fails the `errorString` holds the error details and is dumped via loggers.
- The `initilze` fails and eventually the job fails as well  and calls the `failed` method of scheduled process.
- The failure is logged and the sender sink is pushed with the failed data table name and pipeline resources are deallocated.
