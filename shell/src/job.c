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

#include <job.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct job *init_job(char *command_line, char background)
{
    const int io[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    struct job *j = (struct job *) malloc(sizeof(struct job));

    j->background = background;
    j->first_process = NULL;
    j->pgid = 0;
    j->size = 0;
    j->priority = 0;

    memcpy(j->io, io, sizeof(int) * 3);

    j->command = (char*) malloc((strlen(command_line) + 1) * sizeof(char));
    strcpy(j->command, command_line);

    j->next = NULL;

    return j;
}

void delete_job(struct job *j)
{
    struct process_node *current = j->first_process, *next = NULL;

    while(current) {
        next = current->next;

        if(current->p) {
            if(current->p->argv) {
                int i;
                for(i = 0; current->p->argv[i]; ++i) {
                    free(current->p->argv[i]);
                }
                free(current->p->argv); /* Free token location memory */
            }

            free(current->p);
        }
        free(current);
        current = next;
    }

    free(j->command);
    free(j);
}

void wait_job(struct job *j, struct job *first_job)
{
    pid_t wait_result;
    int status;

    /*signal (SIGCHLD, SIG_DFL);*/

    do {
        wait_result = waitpid(- j->pgid, &status, WUNTRACED);
    } while(!mark_process_status (wait_result, status, first_job)
            && !job_is_stopped(j)
            && !job_is_completed(j));

    /*signal(SIGCHLD, SIG_IGN);*/
}

void signal_continue_job(struct shell_info *s, struct job *j)
{
    tcsetattr(s->terminal, TCSADRAIN, &j->tmodes);
    if(kill(- j->pgid, SIGCONT) < 0)
        perror("almishell: kill (SIGCONT)");
}

void put_job_in_foreground(struct shell_info *s, struct job *j, int cont)
{
    j->background = 'f';

    /* Give control access to the shell terminal to the job */
    if(tcsetpgrp(s->terminal, j->pgid) < 0) {
        perror("almishell: tcsetpgrp");
        return;
    }

    if(cont)
        signal_continue_job(s, j);

    wait_job(j, s->first_job);

    /* Give access to the terminal back to the shell */
    tcsetpgrp(s->terminal, s->pgid);

    /* Restore shell terminal modes */
    tcgetattr(s->terminal, &j->tmodes);
    tcsetattr(s->terminal, TCSADRAIN, &s->tmodes);
}

void put_job_in_background(struct job *j, int cont)
{
    j->background = 'b';

    /* Send the job a continue signal, if necessary.  */
    if (cont)
        if (kill (-j->pgid, SIGCONT) < 0)
            perror ("almishell: kill (SIGCONT)");
}

int launch_job (struct shell_info *s, struct job *j)
{
    struct process_node *node;
    pid_t pid;
    int mypipe[2];
    int io[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    enum SHELL_CMD cmd;

    io[0] = j->io[0];
    for (node = j->first_process; node; node = node->next) {
        /* Set up pipes, if necessary.  */
        if (node->next) {
            if (pipe (mypipe) < 0) {
                perror ("almishell: pipe");
                exit (1);
            }

            /* Redirect output to the pipe */
            io[1] = mypipe[1];
        } else
            io[1] = j->io[1];

        cmd = is_builtin_command(node->p->argv[0]);

        if(cmd == SHELL_NONE) {
            /* Fork the child processes.  */
            pid = fork ();
            if (pid == 0)
                /* This is the child process.  */
                run_process(s, node->p, j->pgid, io, j->background);
            else if (pid < 0) {
                /* The fork failed.  */
                perror ("almishell: fork");
                exit (EXIT_FAILURE);
            } else {
                /* This is the parent process.  */
                node->p->pid = pid;
                if (s->interactive) {
                    if (!j->pgid)
                        j->pgid = pid;
                    setpgid (pid, j->pgid);
                }
            }
        } else {
            FILE *out = io[1] == STDOUT_FILENO ? stdout : fdopen(io[1], "w");

            run_builtin_command(s, out, node->p->argv, cmd);
            node->p->completed = 1;

            if(out != stdout) {
                fclose(out);
                io[1] = STDOUT_FILENO;
            }

            if(cmd == SHELL_EXIT || cmd == SHELL_QUIT) {
                node = NULL;
                io[1] = STDOUT_FILENO;
                break;
            }
        }

        /* Clean up after pipes. */
        if(node->next) {
            if(io[1] != STDOUT_FILENO)
                close(io[1]);

            if(node != j->first_process)
                close(io[0]);

            /* Set the next process input as the pipe input */
            io[0] = mypipe[0];
        }
    }

    j->id = s->tail_job ? s->tail_job->id+1 : 1;

    if(!s->first_job) {
        s->first_job = s->tail_job = j;
        s->first_job->priority = 1;
    } else {
        struct job *curJob = s->first_job;
        int maxPriority = 0;

        while(curJob) {
            maxPriority = curJob->priority > maxPriority ? curJob->priority : maxPriority;

            curJob = curJob->next;
        }
        j->priority = maxPriority+1;

        s->tail_job->next = j;
        s->tail_job = j;
    }

    if(io[0] != STDIN_FILENO)
        close(io[0]);

    if(io[1] != STDOUT_FILENO)
        close(io[1]);

    if(!j->pgid) /* If the pipeline is composed of only built-in commands */
        return 0;

    if (!s->interactive) {
        wait_job (j, s->first_job);
    } else if (j->background == 'f') {
        put_job_in_foreground(s, j, 0);
    } else {
        tcgetattr(s->terminal, &j->tmodes); /* Set up defualt terminal mode */
        put_job_in_background(j, 0);
    }

    return 1;
}

int check_processes(struct job *j)
{
    struct process_node *current;

    if(!j) {
        return 1;
    }

    current = j->first_process;

    while(current) {
        if(!current->p)
            return 0;

        current = current->next;
    }

    return 1;
}

int mark_process_status (pid_t pid, int status, struct job* j)
{
    struct process_node *node;

    if (pid > 0) {
        /* Update the record for the process.  */
        for (; j; j = j->next) {
            for (node = j->first_process; node; node = node->next) {
                if (node->p->pid == pid) {
                    node->p->status = status;
                    if (WIFSTOPPED (status))
                        node->p->stopped = 1;
                    else {
                        node->p->completed = 1;
                        if (WIFSIGNALED (status))
                            fprintf (stderr, "%d: Terminated by signal %d.\n",
                                     (int) pid, WTERMSIG (node->p->status));
                    }
                    return 0;
                }
            }
        }
        fprintf (stderr, "No child process %d.\n", pid);
        return -1;
    } else if (pid == 0 || errno == ECHILD)
        /* No processes ready to report.  */
        return -1;
    else {
        /* Other weird errors.  */
        perror ("almishell: waitpid");
        return -1;
    }
}

/* Check for processes that have status information available,
   without blocking.  */
void update_status(struct job *first_job)
{
    int status;
    pid_t pid;

    do
        pid = waitpid (-1, &status, WUNTRACED|WNOHANG);
    while (!mark_process_status (pid, status, first_job));
}

/* Return true if all processes in the job have stopped or completed.  */
int job_is_stopped (struct job *j)
{
    struct process_node *node;

    for (node = j->first_process; node; node = node->next)
        if (!node->p->completed && !node->p->stopped)
            return 0;
    return 1;
}

/* Return true if all processes in the job have completed.  */
int job_is_completed (struct job *j)
{
    struct process_node *node;

    for (node = j->first_process; node; node = node->next)
        if (!node->p->completed)
            return 0;
    return 1;
}
