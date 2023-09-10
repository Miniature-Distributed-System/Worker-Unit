# Clean Phase

This phase is the first phase of Data Processor stage. This phase is responsible for the cleaning of redundant data in User data in pipeline.
The duplicate data needs to be cleaned to make it faster and easier to process in further phases/stages.

# Working 

- The clean phase is scheduled directly by the data processor.
- The clean phase's `start` method calls `DataCleaner` class's `clean` method which returns non zero when done else 0 if still pending.
- The `clean` takes first record and compares it with the rest of the records of user data and checks to see if any duplicates exist. If found
the clean phase deletes the duplicate records.
- It calls the `FileDataBaseAccess` object helper `deleteDuplicateRecord` which does the above job. (this is scheduled for refactor)
- It does this for every `n-1` records and once done returns 1 on success.
- On `JOB_DONE` the `finalize` method calls the `schedule_validate_phase` which schedules the validation phase of data processor, it exits after releasing resources.
