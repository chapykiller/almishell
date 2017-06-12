#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <termios.h>

struct shell_info {
    int terminal;
    int interactive;
    pid_t pgid;
    struct termios tmodes;
};

/* Ensures proper shell initialization, making sure the shell is executed in
foreground, based on
http://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html#Initializing-the-Shell */

struct shell_info init_shell(void);

#endif /* SHELL_H */
