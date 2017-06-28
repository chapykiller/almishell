#include <job.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct job *init_job(char *command_line)
{
    const int io[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    struct job *j = (struct job *) malloc(sizeof(struct job));

    j->background = 'f';
    j->first_process = NULL;
    j->pgid = 0;
    j->size = 0;

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
        perror("kill (SIGCONT)");
}

void put_job_in_foreground(struct shell_info *s, struct job *j, struct job *first_job, int cont)
{
    /* Give control access to the shell terminal to the job */
    if(tcsetpgrp(s->terminal, j->pgid) < 0) {
        perror("tcsetpgrp");
        return;
    }

    if(cont)
        signal_continue_job(s, j);

    wait_job(j, first_job);

    /* Give access to the terminal back to the shell */
    tcsetpgrp(s->terminal, s->pgid);

    /* Restore shell terminal modes */
    tcgetattr(s->terminal, &j->tmodes);
    tcsetattr(s->terminal, TCSADRAIN, &s->tmodes);
}

void put_job_in_background(struct shell_info *s, struct job *j, struct job *first_job, int cont)
{
    /* TODO */
}

void launch_job (struct shell_info *s, struct job *j, struct job *first_job, int foreground)
{
    struct process_node *node;
    pid_t pid;
    int mypipe[2];
    int io[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

    io[0] = j->io[0];
    for (node = j->first_process; node; node = node->next) {
        /* Set up pipes, if necessary.  */
        if (node->next) {
            if (pipe (mypipe) < 0) {
                perror ("pipe");
                exit (1);
            }

            /* Redirect output to the pipe */
            io[1] = mypipe[1];
        }
        else
            io[1] = j->io[1];

        /* Fork the child processes.  */
        pid = fork ();
        if (pid == 0)
            /* This is the child process.  */
            run_process(s, node->p, j->pgid, io, foreground);
        else if (pid < 0) {
            /* The fork failed.  */
            perror ("fork");
            exit (1);
        } else {
            /* This is the parent process.  */
            node->p->pid = pid;
            if (s->interactive) {
                if (!j->pgid)
                    j->pgid = pid;
                setpgid (pid, j->pgid);
            }
        }

        /* Clean up after pipes. */
        if(node->next) {
            close(io[1]);

            /* Set the next process input as the pipe input */
            io[0] = mypipe[0];
        }

        if(node != j->first_process)
            close(io[0]);
    }

    /* TODO: Inform job launch ? */

    if (!s->interactive) {
        wait_job (j, first_job);
    } else if (foreground) {
        put_job_in_foreground(s, j, first_job, 0);
    }
    else {
        put_job_in_background(s, j, first_job, 0);
    }
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
        perror ("waitpid");
        return -1;
    }
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
