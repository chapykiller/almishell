#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <termios.h>

struct shell_info {
    int terminal;
    int interactive;
    pid_t pgid;
    char *current_path;
    int num_cmd;
    char **cmd;
    struct termios tmodes;
};

/* Ensures proper shell initialization, making sure the shell is executed in
foreground, based on
http://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html#Initializing-the-Shell */

struct shell_info init_shell(void);

void delete_shell(struct shell_info *info);

#endif /* SHELL_H */
