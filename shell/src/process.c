#include <sys/signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <process.h>

/* Create process with default values */
struct process *init_process()
{
    const int io[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    struct process *p = (struct process *) malloc(sizeof(struct process));

    p->argv = NULL;
    p->completed = 0;
    p->pid = -1;
    p->status = 0;
    p->stopped = 0;

    memcpy(p->io, io, sizeof(int) * 3);

    return p;
}

void run_process(struct shell_info *s, struct process *p, pid_t pgid, int fg)
{
    int i;
    pid_t pid;
    const int std_filenos[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

    if(s->interactive) {
        pid = getpid();

        if(pgid == 0) /* Create own process group */
            pgid = pid;

        setpgid(pid, pgid);

        /* If it should run on the foreground */
        if(fg)
            tcsetpgrp(s->terminal, pgid);

        /* Set the handling for job control signals back to the default. */
        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    }

    /* Set the standard input/output channels of the new process.  */
    for(i = 0; i < 3; ++i) {
        if(p->io[i] != std_filenos[i]) {
            if(dup2(p->io[i], std_filenos[i]) < 0) {
                perror("dup2");
            }
            close(p->io[i]);
        }
    }

    for(i = 0; i < SHELL_CMD_NUM; ++i) {
        if(!strcmp(p->argv[0], shell_cmd[i])) {
            i = SHELL_CMD_NUM + 1;
        }
    }

    if(i >= SHELL_CMD_NUM + 2) {
        exit(EXIT_FAILURE);
    }

    execvp(p->argv[0], p->argv);

    /* TODO: Verify if execution state should be reported similarly to runcmd */
    perror("execvp");
    exit(EXIT_FAILURE);
}
