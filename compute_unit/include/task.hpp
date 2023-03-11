#ifndef TASK_H
#define TASK_H

enum job_status {
    JOB_DONE = 0,
    JOB_PENDING,
    JOB_FAILED,
    JOB_FINISHED,
};

enum TaskPriority {
    HIGH_PRIORITY = 0,
    MEDIUM_PRIORITY,
    LOW_PRIORITY,
    DEFAULT_PRIORITY = LOW_PRIORITY,
};

#endif