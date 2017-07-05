/*
ALMiSHELL - A POSIX conformant shell prototype.
Copyright (C) 2017  Henrique C. S. M. Aranha; Lucas E. C. Mello; Lucas H. F. Leal

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <process.h>

/* Create process with default values */
struct process *init_process()
{
    struct process *p = (struct process *) malloc(sizeof(struct process));

    p->argv = NULL;
    p->completed = 0;
    p->pid = -1;
    p->status = 0;
    p->stopped = 0;

    return p;
}

void run_process(struct shell_info *s, struct process *p, pid_t pgid, int io[3], char bg)
{
    int i;
    pid_t pid;
    const int std_filenos[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    struct sigaction sact;

    if(s->interactive) {
        pid = getpid();

        if(pgid == 0) /* Create own process group */
            pgid = pid;

        setpgid(pid, pgid);

        /* If it should run on the foreground */
        if(bg != 'b')
            tcsetpgrp(s->terminal, pgid);

        /* Set the handling for job control signals back to the default. */
        sact.sa_handler = SIG_DFL;
        sigemptyset(&sact.sa_mask);
        sact.sa_flags = 0;
        sigaction(SIGINT, &sact, NULL);
        sigaction(SIGQUIT, &sact, NULL);
        sigaction(SIGTSTP, &sact, NULL);
        sigaction(SIGTTIN, &sact, NULL);
        sigaction(SIGTTOU, &sact, NULL);
        sigaction(SIGCHLD, &sact, NULL);
    }

    /* Set the standard input/output channels of the new process.  */
    for(i = 0; i < 3; ++i) {
        if(io[i] != std_filenos[i]) {
            if(dup2(io[i], std_filenos[i]) < 0) {
                perror("almishell: dup2");
            }
            close(io[i]);
        }
    }

    if(is_builtin_command(p->argv[0]) == SHELL_NONE)
        execvp(p->argv[0], p->argv);

    perror("almishell: execvp");
    exit(EXIT_FAILURE);
}
