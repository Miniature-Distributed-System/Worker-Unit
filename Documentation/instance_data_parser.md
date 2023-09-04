# Instance Data Parser

The instance data parser is a part of the receiver module. This module is resposible for handling the instace data received from the server. It 
parses through the data and extracts useful information and pushes it to the instance repository for future use.

## Working

- The `processInstancePacket` method is called by the main receiver program, this method pulls information like `instanceid`, `data` and `algotype` from the packet directly.
- If any of these data pulls were to fail the packet is concidereded to be corrupted and requests for resend ( returns `P_ERROR` ).
- The `InstanceList` class `addInstance` method is called to register the instance name.
- This method checks if another instance already exists with same name if it does then it must be a updated version of instance therefore the new instance is appended
with a diffrent ID and the `addInstance` returns this internal alias name. Therefore if there are multiple versions of instances worker receives during its
runtime the instance will have diffrent versions of the instance. ( For more refer to Instance List docs )

- Once the above is done it couts the number of columns in instance table and creates an sql command to insert the table into the Sqlite3 database.
- If insertion has any trouble the table is dropped and packet resend is requested.
- If everything succedes then the instance is object is created and stored in memory.
- It finally returns `P_SUCCESS` on completion of this modules task.
