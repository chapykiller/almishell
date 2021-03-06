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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/signal.h>

#include <shell.h>
#include <job.h>

const char *shell_cmd[SHELL_CMD_NUM] = {
    "exit",
    "quit",
    "cd",
    "jobs",
    "fg",
    "bg",
    "almishell"
};

struct shell_info init_shell()
{
    struct shell_info info;
    struct sigaction sact;

    info.terminal = STDIN_FILENO;
    info.interactive = isatty(info.terminal);

    info.current_path = getcwd(NULL, 0);

    /* Setup the sighub handler */
    sact.sa_handler = SIG_IGN;
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;

    /* If the shell is running interactively */
    if(info.interactive) {
        /* Request access to the terminal until we're on foreground. */
        while (tcgetpgrp (info.terminal) != (info.pgid = getpgrp()))
            kill(-info.pgid, SIGTTIN);

        /* Ignore interactive and job-control signals.  */
        sigaction(SIGINT, &sact, NULL);
        sigaction(SIGQUIT, &sact, NULL);
        sigaction(SIGTSTP, &sact, NULL);
        sigaction(SIGTTIN, &sact, NULL);
        sigaction(SIGTTOU, &sact, NULL);
        /*sigaction(SIGCHLD, &sact, NULL);*/

        /* Create our own process group */
        info.pgid = getpid();
        if(setpgid(info.pgid, info.pgid) < 0) {
            perror("almishell: Couldn't put the shell in its own process group");
            exit(EXIT_FAILURE);
        }

        /* Take control of the terminal */
        tcsetpgrp(info.terminal, info.pgid);

        /* Save default terminal attributes for shell */
        tcgetattr(info.terminal, &info.tmodes);
    }

    info.run = 1;
    info.first_job = NULL;
    info.tail_job = NULL;

    return info;
}

void delete_shell(struct shell_info *info)
{
    free(info->current_path);
}

/*  If it's a builtin command, returns its index in the shell_cmd array,
   else returns -1.
*/
enum SHELL_CMD is_builtin_command(const char *cmd)
{
    switch(cmd[0]) {
    case 'e':
        if(strcmp(shell_cmd[SHELL_EXIT], cmd) == 0)
            return SHELL_EXIT;
        break;

    case 'q':
        if(strcmp(shell_cmd[SHELL_QUIT], cmd) == 0)
            return SHELL_QUIT;
        break;

    case 'c':
        if(strcmp(shell_cmd[SHELL_CD], cmd) == 0)
            return SHELL_CD;
        break;

    case 'j':
        if(strcmp(shell_cmd[SHELL_JOBS], cmd) == 0)
            return SHELL_JOBS;
        break;

    case 'f':
        if(strcmp(shell_cmd[SHELL_FG], cmd) == 0)
            return SHELL_FG;
        break;

    case 'b':
        if(strcmp(shell_cmd[SHELL_BG], cmd) == 0)
            return SHELL_BG;
        break;

    case 'a':
        if(strcmp(shell_cmd[SHELL_ALMISHELL], cmd) == 0)
            return SHELL_ALMISHELL;
        break;

    default:
        break;
    }

    return SHELL_NONE;
}

void run_jobs(struct shell_info *sh)
{
    struct job *it = sh->first_job;
    struct job *curJob, *minusJob, *plusJob;
    curJob = plusJob = minusJob = sh->first_job;

    update_status(sh->first_job);

    while(curJob) {
        if(curJob->priority > plusJob->priority) {
            if(curJob->priority == plusJob->priority+1) {
                minusJob = plusJob;
            }
            plusJob = curJob;
        }
        else if(curJob->priority == plusJob->priority - 1) {
            minusJob = curJob;
        }

        curJob = curJob->next;
    }

    while(it) {
        printf("[%d]%c  ", it->id, (plusJob->id==it->id ? '+' : (minusJob->id==it->id ? '-' : ' ')));

        if(job_is_completed(it)) {
            struct process_node *last = it->first_process;

            /* Loop until variable last holds the last process */
            while(last->next != NULL)
                last = last->next;

            printf("Done");

            if(last->p->status != 0)
                printf("(%d)", last->p->status);
        } else if(job_is_stopped(it)) {
            printf("Stopped");
        } else { /* Job is running */
            printf("Running");
        }

        printf("\t\t\t%s%s\n", it->command, it->background == 'b' ? " &" : "");
        it = it->next;
    }
}

void fg_bg(struct shell_info *sh, char **args, int id)
{
    int i;
    struct job *current;
    struct process_node *node_p;
    struct job *curJob, *maxPriority;
    curJob = maxPriority = sh->first_job;

    while(curJob) {
        maxPriority = curJob->priority > maxPriority->priority ? curJob : maxPriority;

        curJob = curJob->next;
    }

    if(!args[1]) {
        i = maxPriority->id;
    } else {
        i = atoi(args[1]);
    }

    if(i < 1) {
        printf("almishell: %s: %s: no such job\n", args[0], args[1] ? args[1] : "current");
        return;
    }

    current = sh->first_job;
    while(current) {
        if(current->id == i) {
            int max = maxPriority->priority;
            curJob = sh->first_job;
            while(curJob) {
                if(curJob->priority > current->priority) {
                    --curJob->priority;
                }

                curJob = curJob->next;
            }

            current->priority = max;

            printf("%s\n", current->command);

            for (node_p = current->first_process; node_p; node_p = node_p->next)
                node_p->p->stopped = 0;

            if(id == SHELL_FG)
                put_job_in_foreground(sh, current, 1);
            else
                put_job_in_background(current, 1);

            return;
        }
        current = current->next;
    }

    printf("almishell: %s: %s: no such job\n", args[0], args[1] ? args[1] : "current");
}

void run_builtin_command(struct shell_info *sh, FILE *out, char **args, int id)
{
    switch(id) {
    case SHELL_EXIT:
    case SHELL_QUIT:
        sh->run = 0;
        break;

    case SHELL_CD:
        if(args[1]) {
            if(chdir(args[1]) < 0)
                perror("almishell: cd");

            free(sh->current_path);
            sh->current_path = getcwd(NULL, 0);
            setenv("PWD", sh->current_path, 1);
        }
        break;

    case SHELL_JOBS:
        run_jobs(sh);
        break;

    case SHELL_FG:
    case SHELL_BG:
        fg_bg(sh, args, id);
        break;

    case SHELL_ALMISHELL:
        fprintf(out, "\"Os alunos tão latindo Michel, traz a antirábica.\"\n");
        fflush(out);
        break;

    default:
        fprintf(out, "almishell: invalid command\n");
        fflush(out);
        break;
    }
}
