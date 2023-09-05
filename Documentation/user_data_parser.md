# User Data Parser

The user data parser is a paert of the receiver module that is responsible for handling the user data received from the server.
The User data needs to be parsed and validated. This data is converted into a pipeline for further processing.

## Working

- The `processDataPacket` method is called by passing the `tableID` and `DataProcessContainer` object inited by the receiver.
- This method pulls `tableid`, `data`, `instancetype` and `priority` from the packet any error in this it throws `P_ERROR`.
- A file is created with `tableid` as the name of the file.
- The `instancetype` is used to determine the type of Instance which will be used for processing the user data from the instance list.
- Therefore we make a reference to that Instance rule by incrementing the reference count.
- Then we retreive the instance internal name (current version).
- Using the `FileDataBaseAccess` helper methods we retrive the column and row count metadata.
- After this the User data object is constructed.
- Finally the data is deallocted and returns `P_SUCCESS`.
