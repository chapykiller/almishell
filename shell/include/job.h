#ifndef JOB_H
#define JOB_H

#include <process.h>
#include <shell.h>
#include <termios.h>

struct process_node {
    struct process *p;
    struct process_node *next;
};

/* Preliminary job representation, for a job with a single process */
struct job {
    struct process_node *first_process;
    char background;
    pid_t pgid;
    struct termios tmodes;
    size_t size;
};

struct job *init_job(void);

void delete_job(struct job *j);

void wait_job(struct job *j);

void put_job_in_foreground(struct shell_info *s, struct job *j, int cont);

void launch_job(struct shell_info *s, struct job *j, int foreground);

#endif /* JOB_H */