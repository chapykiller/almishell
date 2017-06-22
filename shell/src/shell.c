#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>

#include <shell.h>

struct shell_info init_shell()
{
    struct shell_info info;

    info.terminal = STDIN_FILENO;
    info.interactive = isatty(info.terminal);

    info.current_path = getcwd(NULL, 0);

    info.num_cmd = 6;
    info.cmd = (char**) malloc(info.num_cmd * sizeof(char*));
    info.cmd[0] = "exit";
    info.cmd[1] = "quit";
    info.cmd[2] = "cd";
    info.cmd[3] = "jobs";
    info.cmd[4] = "fg";
    info.cmd[5] = "bg";

    /* If the shell is running interactively */
    if(info.interactive) {
        /* Request access to the terminal until we're on foreground. */
        while (tcgetpgrp (info.terminal) != (info.pgid = getpgrp()))
            kill(-info.pgid, SIGTTIN);

        /* Ignore interactive and job-control signals.  */
        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);

        /* Create our own process group */
        info.pgid = getpid();
        if(setpgid(info.pgid, info.pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(EXIT_FAILURE);
        }

        /* Take control of the terminal */
        tcsetpgrp(info.terminal, info.pgid);

        /* Save default terminal attributes for shell */
        tcgetattr(info.terminal, &info.tmodes);
    }

    return info;
}

void delete_shell(struct shell_info *info) {
    free(info->current_path);
    free(info->cmd);
}
