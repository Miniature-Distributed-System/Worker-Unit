#ifndef TASK_H
#define TASK_H

/// This type is used for setting current job state in the pipeline
enum JobStatus {
    JOB_DONE        = 1 << 0,
    JOB_RUNNING     = 1 << 1,
    JOB_PENDING     = 1 << 2,
    JOB_WAITING     = 1 << 3,
    JOB_FAILED      = 1 << 4,
    JOB_FINALIZED   = 1 << 5,
    JOB_DONE        = 1 << 6
};

enum TaskPriority {
    NON_PREEMTABLE = 0,
    HIGH_PRIORITY,
    MEDIUM_PRIORITY,
    LOW_PRIORITY,
    DEFAULT_PRIORITY = LOW_PRIORITY,
    NR_TASK_PRIORITY = DEFAULT_PRIORITY,
};

#endif