# Worker Unit

- This unit is responsible for processing the user data.
- It communicates the server, regestering itself with the sevver and downloading rules for processing of user data.
- The user data is passed to worker units running on client machines, this data is converted inot pipelines and processed phase wise.
- Depenending on the phase the data can be processed serially or parallelly.
- The resources on client machine is adjusted by the client during startup and the worker unit manages processing within these constarints.
- Once the final phase is complete the user data result is sent to server and space allocated for user data on client is released.
- The worker unit is listening to server for collecting user data and sending user data results serially.
