# File Database Access

This service can be included to access the File database or create a new file database. The file database is similar to CSV. The
file can be accessed by passing `filename` string and Access type macro. The access type can open file in read only or read write
mode.

#### `FileDataBaseAccess(std::string fileName, FileAccessType accessMacro)`

- This constructor needs to be called while creating a new object. The file can either be opened or created based on use case.
- The File cannot be created in `READ_FILE` mode. It can only be created in `RW_FILE` mode.
- If the file open fails an exception is genereted and failure is logged by Logger.
- When object goes out of scope the object destructor is called and changes are commited.

## File Read Mode only methods (Fetch methods)

#### `getBlob()`

- fetches the data as a single formatted string blob.

#### `getTotalRows()`

- Fetches the total number of rows in the current database.

#### `getTotalColumns()`

- Fetches the total number of columns excluding the primary key column in current database.

#### `getRowValueList(int rowIndex)`

- Fetches the all values of the current row as a vector.

#### `getColumnNamesList()`

- Fetches the Column headers names as a vector of values.

#### `getRowValue(int rowIndex, int columnIndex)`

- Fetches the value at `rowIndex` and `columnIndex` in database matrix. 

## File Read Write Mode methods

#### `writeBlob(std::string data)`

- This method can be called to  write a bulk of data at once.

#### `writeRowValue(std::string value, int rowIndex, int columnIndex)`

- This method writes a Row's value into the File database. It inserts the `value` into the row index mentioned under the column
number.
- The method returns error if the file doesnt exist.
- The overflow and underflow is taken care of here

#### `deleteDuplicateRecords(int rowIndex)`

- This method can be used to purge duplicate records. The row index needs to be mentioned and it checks against all duplicate
rows and deleted them reducing the size of database

#### `dropFile()`

- This deletes the file and it cannot be reverted.

#### `commitChanges()`

- This saves all changes done to the target files. Its also executed automatically when the object destructs.
