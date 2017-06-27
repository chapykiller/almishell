#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <termios.h>

enum SHELL_CMD {
    SHELL_EXIT,
    SHELL_QUIT,
    SHELL_CD,
    SHELL_JOBS,
    SHELL_FG,
    SHELL_BG,
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
};

/* Ensures proper shell initialization, making sure the shell is executed in
foreground, based on
http://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html#Initializing-the-Shell */

struct shell_info init_shell(void);

void delete_shell(struct shell_info *info);

#endif /* SHELL_H */
