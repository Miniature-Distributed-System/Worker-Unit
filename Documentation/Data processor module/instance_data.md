# Instance Data

This is responsible for initlizing the instance data set. The instace related data is collected for the given user data. This data is used to 
pre-validate the user data. Any failures here causes the pipeline to be aborted.

# Working

- The `initlizeData` method is called which is main method of `InstanceData` class.
- The method calls the `validateColumns` method which validates the column name of the User data headers with the Instance Rules. It also reorders
columns headers if required.
- The column count and name must be the same else reported as failure.
- The `SqlDataBaseAccess` class `getColumnValues` gets the column names in the order of User data columns.
- The `initlizeData` method returns once above is completed.
- The `InstanceData` offers another method `getPossibleFeilds` which gets the possible values that column values can have. If they don't match then the value
can be invalidated and replaced with 'Not for concideration values' telling compute unit to not concider those values during compute. 
