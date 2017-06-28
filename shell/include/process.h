#ifndef PROCESS_H
#define PROCESS_H

#include <unistd.h>

#include <shell.h>

/* Structure representing a process, from glibc manual*/
struct process {
    char **argv;                /* for exec */
    pid_t pid;                  /* process ID */
    char completed;             /* true if process has completed */
    char stopped;               /* true if process has stopped */
    int status;                 /* reported status value */
};

struct process *init_process(void);

/* NOTE: Should be called after fork */
void run_process(struct shell_info *s, struct process *p, pid_t pgid, int io[3], char bg);

#endif /* PROCESS_H */
