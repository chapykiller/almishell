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
    int priority;
};

struct job *init_job(char *command_line, char background);

void delete_job(struct job *j);

void wait_job(struct job *j, struct job *first_job);

void put_job_in_foreground(struct shell_info *s, struct job *j, int cont);

void put_job_in_background(struct job *j, int cont);

int launch_job(struct shell_info *s, struct job *j);

int check_processes(struct job *j);

int mark_process_status(pid_t pid, int status, struct job* j);

void update_status(struct job *first_job);

int job_is_stopped(struct job *j);

int job_is_completed(struct job *j);

#endif /* JOB_H */
