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
    int id;
    char *command;
    struct process_node *first_process;
    char background;
    pid_t pgid;
    struct termios tmodes;
    size_t size;
    struct job *next;
    int io[3];
};

struct job *init_job(char *command_line, char background);

void delete_job(struct job *j);

void wait_job(struct job *j, struct job *first_job);

void put_job_in_foreground(struct shell_info *s, struct job *j, struct job *first_job, int cont);

void put_job_in_background(struct shell_info *s, struct job *j, struct job *first_job, int cont);

void launch_job(struct shell_info *s, struct job *j, struct job *first_job);

int check_processes(struct job *j);

int mark_process_status(pid_t pid, int status, struct job* j);

int job_is_stopped(struct job *j);

int job_is_completed(struct job *j);

#endif /* JOB_H */
