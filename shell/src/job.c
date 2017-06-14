#include <job.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

struct job *init_job()
{
    struct job *j = (struct job *) malloc(sizeof(struct job));

    j->background = 'f';
    j->first_process = NULL;
    j->pgid = 0;
    j->size = 0;

    return j;
}

void delete_job(struct job *j)
{
    struct process_node *current = j->first_process, *next = NULL;

    while(current) {
        next = current->next;
        free(current->p->argv); /* Free token location memory */
        free(current->p);
        free(current);
        current = next;
    }

    free(j);
}

void wait_job(struct job *j)
{
    int status;
    size_t terminated = 0;

    while(terminated++ < j->size) {
        if(waitpid(- j->pgid, &status, 0) < 0) {
            perror("waitpid");
            break;
        }
    }
}

void signal_continue_job(struct shell_info *s, struct job *j)
{
    tcsetattr(s->terminal, TCSADRAIN, &j->tmodes);
    if(kill(- j->pgid, SIGCONT) < 0)
        perror("kill (SIGCONT)");
}

void put_job_in_foreground(struct shell_info *s, struct job *j, int cont)
{
    /* Give control access to the shell terminal to the job */
    tcsetpgrp(s->terminal, j->pgid);

    if(cont)
        signal_continue_job(s, j);

    wait_job(j);

    /* Give access to the terminal back to the shell */
    tcsetpgrp(s->terminal, s->pgid);

    /* Restore shell terminal modes */
    tcgetattr(s->terminal, &j->tmodes);
    tcsetattr(s->terminal, TCSADRAIN, &s->tmodes);
}

void launch_job (struct shell_info *s, struct job *j, int foreground)
{
    struct process_node *node;
    pid_t pid;
    int mypipe[2], infile;

    /* TODO: Input redirection for the whole pipe or process ? */
    infile = j->first_process->p->io[0];
    for (node = j->first_process; node; node = node->next) {
        /* Set up pipes, if necessary.  */
        if (node->next) {
            if (pipe (mypipe) < 0) {
                perror ("pipe");
                exit (1);
            }
            if(node->p->io[1] != STDOUT_FILENO) {
                close(node->p->io[1]);
                node->p->io[1] = mypipe[1];
            }
        }

        /* Fork the child processes.  */
        pid = fork ();
        if (pid == 0)
            /* This is the child process.  */
            run_process(s, node->p, j->pgid, foreground);
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

        /* Clean up after pipes.  */
        if (infile != j->first_process->p->io[0])
            close (infile);

        if (node->next)
            close (node->p->io[1]);

        infile = mypipe[0];
    }

    /* TODO: Inform job launch ? */

    if (!s->interactive)
        wait_job (j);
    else if (foreground)
        put_job_in_foreground(s, j, 0);
    /* TODO: else
        put_job_in_background (j, 0); */
}
