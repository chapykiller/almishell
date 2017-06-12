#include <sys/signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <process.h>

void run_process(struct shell_info *s, struct process *p, int fg)
{
    int i;
    pid_t pid;
    const int std_filenos[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

    if(s->interactive) {
        pid = getpid();

        /* Create own process group */
        setpgid(pid, pid);

        /* If it should run on the foreground */
        if(fg)
            tcsetpgrp(s->terminal, pid);

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

    execvp(p->argv[0], p->argv);

    /* TODO: Verify if execution state should be reported similarly to runcmd */
    perror("execvp");
    exit(EXIT_FAILURE);
}
