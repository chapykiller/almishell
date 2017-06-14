#ifndef JOB_H
#define JOB_H

#include <process.h>
#include <shell.h>

struct process_node {
    struct process *p;
    struct process_node *next;
};

/* Preliminary job representation, for a job with a single process */
struct job {
    struct process_node *first_process;
    char background;
};

struct job *init_job(void);

void delete_job(struct job *j);

void put_job_in_foreground(struct shell_info *s, struct job *j, int cont);

void run_job(struct shell_info *s, struct job *j, int fg);

int check_processes(struct job *j);

#endif /* JOB_H */
