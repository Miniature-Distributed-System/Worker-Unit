## Creating and Adding Custom Algorithms

Creating custom algorithms for worker/compute unit has a set method and rules to be followed. These rules are made as easy and understandable by this guide.
The `example.cpp` and `example.hpp` give us an idea of base structure of code. We need to build our algorithm on top of this structure. 
Algorithms have been divided into:-
1. Split Stage
2. Aggrigate Stage.


## Split Stage
For creating your own custom algorithms we have set of rules.
1. The Algorithms all have initialization methods during which the Algirithms persistant data structures will be allocated.
2. The Algorithms will all have start, pause and end methods.
3. The Algorithms shoud clean up memory it allocates during its runtime before finishing up.

The below code shows the standard structure of Algorithms:-

```cpp
JobStatus example_start_process(void *data){
    Log().info(__func__, "Example start process");
    return JOB_DONE;
}

JobStatus example_pause_process(void *data){
    Log().info(__func__, "Example pause process");
    return JOB_DONE;
}

void example_end_process(void *data){
    Log().info(__func__, "Example end process");
}

struct ProcessStates* example_proc = new ProcessStates{
    .start_proc = example_start_process,
    .pause_proc = example_pause_process,
    .end_proc = example_end_process
};
```

### Initilization
This is the start point of the Algorithm. If this point fails the whole pipeline collapses and dataset won't be processed.

```cpp

struct ProcessStates* example_proc = new ProcessStates{
    .start_proc = example_start_process,
    .pause_proc = example_pause_process,
    .end_proc = example_end_process
};

AlgorithmExportPackage init_your_algorithm(std::string tableId, std::string instanceId, int start, 
  int end, int columns)
{
    YourAlgorithmClass *obj = new YourAlgorithmClass(tableId, instanceId, start, end, columns);
    AlgorithmExportPackage finalizePackage(example_proc, ce);
    return finalizePackage;
}
```

The `init_your_algorithm` will be called during initilization.
In order for the method to be called the above `init_your_algorithm` must be added to `algorithm_scheduler.hpp` array of pointers as shown below:- 
```cpp
static struct AlgorithmExportPackage (*avial_algo[TOT_ALGO])(std::string tableId, std::string instanceId, 
  int start, int end, int columns) = {
        init_example_algo,
        init_your_algorithm
};
```
Note here that `TOT_ALGO` must be updated with correct array size and the order of your algorithm here matters a lot. This order will determine which algorithm will be
called for processing by Algorithm Scheduler. So don't make any mistakes here.

The Algorithm Scheduler will pass `tableId` which will be the access name of the Dataset to be processed, `InstanceId` will be the name of the Rules Dataset which you 
may or may not need. The `start` and `end` are your to be processed Dataset's start row and end row indexes. All these values must be saved in your Object during init.
The `start` and `end` are necessary as the dataset will be split based on number of threads available, In other words multiple instances of your algorithm will run
simultaniously.

After saving the arguments in Object we need to create a `AlgorithmExportPackage` object whcih will package your `ProcessState` and your algorithm Object and finnally
pass that object as return value to Algorithm Scheduler.

### Start Process
The `.start_proc` Will be your start point where in you can pull in data from database and do computaion.
```cpp
.start_proc = example_start_process,
```
The `start_proc` will pass a `void* data` which will be the Algorithm data structure you passed during initilization.
In order to pull data from database we must initlize the `FileDataBaseAccess` method with the provided tableId
```cpp
FileDataBaseAccess fileDataBaseAccess = new FileDataBaseAccess(tableId, READ_FILE);
```
The above code initilizes the database in read mode 'NOTE: do not access the database in read/write mode as we may end up overwriting file in that mode and breaking 
concurrency.'
We can also pull Dataset Rules from sqlite database as below:-
```cpp
std::string* columnValues = sqliteDatabaseAccess->getColumnNames(instanceId, columnCount);
```
SQLlite accessor provides many methods for custom access of tables you can look up all its methods in `services/sqlite_database_access.hpp`.

The Start Process should return `JobState` signals to the scheduler it may be `JOB_PENDING` or `JOB_DONE`. The scheduler will call the Start Process iteratively as 
long as we pass the return values `JOB_PENDING` until we return `JOB_DONE` the algorithm will be stuck in Start Process state.
The Records must be processed only **ONE ROW** at a time. This is done so as to allow other tasks to execute concurrently and finish simultaniously.

### Pause Process
The `.pause_process` will be called by the scheduler when your algorithm runs out of allocated time and will be queued back in a round robin fashion. Here you can send
the Sender unit a message to server telling it that Dataset is still being processed updating server about the status of Processing.
```cpp
YourObject *yourSavedObj = (YourObject*)data;
senderSink.pushPacket("", yourSavedObj->tableId, RESET_TIMER, DEFAULT_PRIORITY);
```

### End Process
This is the final part of Split Stage computing. where the `.start_process` would have sent `JOB_DONE` signal to scheduler. During this part of the algorithm we have
to send the saved results to the Aggrigator. The Aggrigator that we create must have a data structure that will package all the results and send it to Algorithm
Scheduler which will handle these packages in a vector. The vector of data structures will be given as arguments to the Aggrigator.

```cpp
void candidate_elimination_end(void *data)
{
    YourObject *obj = (YourObject*)data;
    // This is your own struct/class
    ResultPacakge *resObj = new ResultPackage(obj->getResult());
    update_algo_result(obj->tableId, resObj);
    // Before exiting cleanup your mess
    delete obj;
}
```

This marks the end of Split Stage, now comes the aggrigator.
<br></br>
## Aggrigator
The Aggrigator will merge your Split results as final result. The aggrigator will follow same rules as Above with the only exceptions that we send `JOB_DONE` as
the only return value and we don't process record wise we aggrigate all results at once. The scheduler will only run the aggrigator once.
The aggrigator will follow same code structure just like Split Stage.

### Initilization
The initlization of Aggrigator is same as above with few changes. The `init_algorithm_aggrigator` will need to be added to a diffrent array of function pointer
```cpp
static struct AlgorithmExportPackage (*avialable_finalize_algo[TOT_ALGO])(std::vector<void *> resultVectors, 
    std::string tableId, TaskPriority taskPriority) = {
    init_example_finalize,
    init_algorithm_aggrigator
};
```
Again here the aggrigator function must be ordered as the Split stage method both must have same index.

The `init_algorithm_aggrigator` will receive diffrent parametersets.
It will receive Vector of DataStructures with saved values from all Split Stages. This Vector should be iterated over to aggrigate results in Start Process. The
`tableID` feild will give dataset access name and `priority` will give the priority of dataset all of these need to be saved in your object.
The saved object will packaged into `AlgorithmExportPackage` and returned.

```cpp
AlgorithmExportPackage init_algorithm_aggrigator(std::vector<void*> resultVectors, std::string tableId, 
  TaskPriority priority)
{
    ResultFinalize *resultFinalize = new ResultFinalize(resultVectors, tableId, priority);
    return AlgorithmExportPackage(result_finalize_proc, resultFinalize);
}
```

### Start Process
The Start Process will aggrigate results of all Split stages. which will be in from vectors of data.
```cpp
JobStatus ce_finalize_start_process(void *data)
{
    if(!data)
        JOB_FAILED;
    ResultFinalize *resultFinalize = (ResultFinalize*)data;
    resultFinalize->aggrigateResults();
    return JOB_DONE;
}
```

### End Process
The End Process will send the result to Sender Unit and cleanup.
```cpp
AlgorithmFinalize *finalize = (AlgorithmFinalize*)data;
  std::string finalString = finalize->getFinalResult();
  Log().info(__func__, "final result is:", finalString);
  senderSink.pushPacket(finalString, finalize->tableId, FRES_SEND, finalize->taskPriority);
}
```

NOTE:- I have not mentioned pause process we still need a dummy method to be mentioned as its necessary at the moment will be deleted soon.

With this it would mark the end of pipeline and results will be safely delivered to the server.

## Testing
Now to Test this we have a specialized websocket in `/test` directory which simulates the server with test data. 
The Array is of form json. We can replace the data in the `data` feild with out dataset and rules to test out the algorithm.
- `data`: Contains either Rule dataset or process dataset
- `instanceId`: Name of Rule dataset(can put some dummy value)
- `priority`: Priority of dataset(put some dummy value here)
- `tableid`: Contains the name of to be processed dataset
- `algotype`: Index of your algorithm

```cpp
std::string dataArr[3] = {
    "{\"head\":1,\"id\":\"cu0000\",\"body\":{}}",
    "{\"head\":2,\"id\":\"cu0000\",\"body\":{\"instanceid\":\"instance1\",\"algotype\":1,\"data\":\"sky,airtemp,humidity,wind,water,forecast,enjoysport\\nsunny,warm,normal,strong,warm,same,yes\\ncloudy,cold,high,weak,same,change,no\\nrainy,normal,NaN,NaN,cool,NaN,NaN\\n\"}}",
    "{\"head\":2,\"id\":\"cu0000\",\"body\":{\"tableid\":\"myTable2\",\"priority\":2,\"instancetype\":\"instance1\",\"data\":\"sky,airtemp,humidity,wind,water,forecast,enjoysport\\nsunny,warm,normal,strong,warm,same,yes\\nsunny,warm,high,strong,warm,same,yes\\nrainy,cold,high,strong,cool,change,no\\nsunny,warm,high,strong,cool,change,yes\\nsunny,warm,normal,strong,warm,change,yes\\nsunny,warm,normal,weak,warm,same,yes\\nrainy,cold,high,strong,warm,same,no\\nsunny,warm,normal,weak,warm,change,yes\\nrainy,cold,normal,strong,cold,same,no\\nsunny,warm,normal,strong,warm,same,yes\"}}"
};
```
End of record/line is delimited with newline character `\n`.

- First element of array is the handshake packet so don't touch that. 
- Second element is Rule Packet which needs to be replaced with your rules. 
- The Third/last element is the To be processed Dataset to be replaced with your dataset. 

Run the websocket and then the websocket test server. The websocket will automatically feed all data and if you want to pop out all queued packets of Worker/Compute
unit just send some dummy values in 'write data' input of websocket to check all packets received from Worker.

The results can also be viewed in terminal which is coloured to help identify processes in the worker.
