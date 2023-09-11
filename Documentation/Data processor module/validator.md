# Data Validator

This module is responsible for validation of user data. It compares user data each row value with the instance rules and checks for correctness.
If data is found to be incorrect the row data value is ignored. This module runs parellely. It is scheduled by the data tracker. the data tracker splits
the user data into multiple parts depending on the number of threads available to the scheduler. All these parts validate diffrent parts of the user data simultaniously.

# Working

- The validator is scheduled by the data tracker.
- The `start` method calls the `processFeild` of `ValidateData` object, it returns non zero number on completion.
- The  `processFeild` method gets all possible values for a particular column(from instance rules).
- It uses this data to compare each column and check if it matches the instance rules, if not conforming to rule then the value is replaced
with ignore value (this value usually NaN will tell the algorithm to ignore the data and skip to next row).
- This data is written directly to user data file.
- Each column is individually compared and the old row data is replaced with the modifled row data.
- Once all rows have been traversed the valdator exist with return value 1.
- The `finalize` method updates the data tracker which notifies the data tracker that data validator splits have completed thier jobs.
