#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>

#include <shell.h>

const char *shell_cmd[SHELL_CMD_NUM] = {
    "exit",
    "quit",
    "cd",
    "jobs",
    "fg",
    "bg"
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
}
