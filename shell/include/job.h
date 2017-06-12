#ifndef JOB_H
#define JOB_H

#include <process.h>
#include <shell.h>

/* Preliminary job representation, for a job with a single process */
struct job {
    struct process *first_process;
};

void put_job_in_foreground(struct shell_info *s, struct job *j, int cont);

void run_job(struct shell_info *s, struct job *j, int fg);

#endif /* JOB_H */
