#ifndef TASK_H
#define TASK_H

enum job_status {
    JOB_DONE = 0,
    JOB_PENDING,
    JOB_FAILED,
    JOB_FINISHED,
};

enum task_priority {
    HIGH_PRIOR = 0,
    MED_PRIOR,
    LOW_PRIOR,
    DEFAULT_PRIOR = LOW_PRIOR,
};

#endif