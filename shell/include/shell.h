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
    int plusJob;
    int minusJob;

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
