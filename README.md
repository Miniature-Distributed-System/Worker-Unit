# Worker Unit

The worker unit is the edge nodes of the Miniature Distributed systems. These units are individual silos and work indipendent of other workers. The worker is installed on user
Host machines which are commodity systems, aka resources are sparse and should be managed.

### Init
1. The user is prompted in CLI for worker configuration.
2. The user must enter the IP Address of the server for connection.
3. Total threads that is to be made available to the worker must be mentioned.
### Start
1. The worker unit establishes connection with the server and initiates a handshake.
2. The worker is assigned a ID by the server for worker identification server side.
3. The Worker then waits for data from the server.
4. The server sends processable data to the worker.
5. The data sent out conforms to a template and rule associated with it which is fetched by the worker initially.
6. Once worker has all necessary rules and templates it makes itself available for processing load.
7. The server sends out load and the worker creates pipeline for each load.
8. The worker processes the load stage wise serially but multiple pipelines are executed parellely on threads. The pipelines are interleaved to make processing faster.
9. The worker buffers the result in server once done with processing.
10. The worker sends out any failed or error data as error packet to the server for re-sending or error handeling.


- The worker has its own scheduler and task pool for managing worker processes.
- The worker is built modular and is multi staged.
- Websockets are always up and responding to server, if websocket doesnt respond to server then server will timeout worker and concider worker as dead and all data regarding
it is ditched and no previous data with old worker tag is accepted.
- Worker will keep pining server if server connection fails.
- Worker/Server packets are at the moment not encrypted but will be in the future.
