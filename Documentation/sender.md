# Sender Sink

This module is responsible for queuing of data to be sent to server. The module takes in data and pops out the oldest data. Its primarly accesed by
modules which want to update data to server and socket which is resposible for sending of this data to server.

# Working

Below is worker packet representation
```
  head: Worker Status Code(int),
  id: Worker ID(string),
  body
  {
    id: Data ID(string),
    priority: Data Priority(int),
    data: Result(string),
  },
  stats: Statistics Engine Data(string)
```

### pushPacket

- The `pushpacket` method is by modules to queue data to be sent to the server. It takes `tableID`, `SenderDataType` which is used to determine
what type of data is being sent to the server and `TaskPriority` which bubbles up the packet if higher priority and also lets server know its higher priority
and `data` which needs to be of type string.
- The packet is pushed into `ForwardStack` object using `pushFront` and `push` methods. If its a `SEIZE` signal then the data is pushed into front of the queue
else its pushed to rear of queue.
- The `Socket` flag is set to `SOC_SETQS` quick send mode ( Needs refactoring).

### popPacket

- The `popPacket` method is used to pop top most packet from queue and return that packet.
- This method first checks if the worker has an ID, if its doesnt have an ID then the worker just started and has nothing in the queue therfore it
sends default(empty) packet.
- else it calls the `create_packet` method for creation of packet from queued data.
- It returns the packet returned by the above method to Socket.

### create_packet

- This method creates the packet from the queued data.
- It compiles information from the popped data of queue, it first gives `head` information based on socket status.
- After that it adds worker ID to the packet in `id` feild.
- The status code is extracted and using this status code the `id`, `priority` and `data` sub feilds of `body` is given. The `id` is the data id and
data is the result data of that data ID.
- Before returning the `stats` feild is added to end of every data which is Status Engine data.
