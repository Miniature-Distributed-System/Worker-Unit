# Socket

This module receives the packet/data from the server and reposnds back with its own packet/data. Socket connects the sever to the worker. This module
is very important and any failure here causes the worker to be shutdown. The socket is fed with hostname and port number during startup of worker.

## Working

- When the socket module starts the socket sets up socket thread.
- This thread runs in a loop, while executing another socket child thread that is responsible for socket functionality (receiving/sending data to server).
- The socket thread has two states Normal mode and Quicksend mode.
- In normal mode which is default the socket sends the data to server and receives data serially.
- The server once it receives data from worker waits for data for the worker and doesnt release the worker until data is received from upper layers.
- When the socket detects that the sender module has more than one packet queued in the stack it switches the socket to Quicksend mode.
- The socket when in quicksend mode can setup another child thread along side the Normal mode thread.
- While the Normal mode thread waits for the data from server the Quicksend mode thread sends the next packet in the stack to the server and also informs the server to not wait for data and release the worker.
- This causes the server to not wait for further data for the worker in question and instead quickly sends whatever is available(even empty packets) to the worker.
- This also allows the worker to empty its sender stack so it can deliver all messages to server without delay.
- Only one quick send thread can be executed if for whatever reason the quick send thread gets locked up with server the worker will experinece delays which is an undesired scenario
- The quick send mode is exited once the sender stack is empty and the server is also sent this change in state.
