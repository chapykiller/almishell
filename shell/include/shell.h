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

#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <termios.h>

#include <stdio.h>

/* Forward declaration */
struct job;

enum SHELL_CMD {
    SHELL_EXIT,
    SHELL_QUIT,
    SHELL_CD,
    SHELL_JOBS,
    SHELL_FG,
    SHELL_BG,
    SHELL_ALMISHELL,
    SHELL_CMD_NUM,
    SHELL_NONE
};

const char *shell_cmd[SHELL_CMD_NUM];

struct shell_info {
    int terminal;
    int interactive;
    pid_t pgid;
    char *current_path;
    struct termios tmodes;
    int run;

    struct job *first_job, *tail_job;
};

/* Ensures proper shell initialization, making sure the shell is executed in
foreground, based on
http://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html#Initializing-the-Shell */

struct shell_info init_shell(void);

void delete_shell(struct shell_info *info);

/*  If it's a builtin command, returns its index in the shell_cmd array,
   else returns -1.
*/
enum SHELL_CMD is_builtin_command(const char *cmd);

void run_jobs(struct shell_info *sh);

void fg_bg(struct shell_info *sh, char **args, int id);

void run_builtin_command(struct shell_info *sh, FILE *out, char **args, int id);

#endif /* SHELL_H */
