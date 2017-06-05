#include <runcmd/runcmd.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* If function runcmd is called in non-blocking mode, then function
pointed by rcmd_onexit is asynchronously evoked upon the subprocess
termination. If this variable points to NULL, no action is performed.
*/

void (*runcmd_onexit)(void) = NULL;

struct sigaction act, old_act;

void runcmd_adapter(int dummy);

void setup_signal_handlers()
{
    act.sa_handler = runcmd_adapter;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, &old_act);
}

void restore_default_handlers()
{
    sigaction(SIGCHLD, &old_act, &act);
}

void runcmd_adapter(int dummy)
{
    if(runcmd_onexit)
        runcmd_onexit();
    restore_default_handlers();
}

/* Executes 'command' in a subprocess. Information on the subprocess execution
   is stored in 'result' after its completion, and can be inspected with the
   aid of macros made available for this purpose. Argument 'io' is a pointer
   to an integer vector where the first, second and third positions store
   file descriptors to where standard input, output and error, respective,
   shall be redirected; if NULL, no redirection is performed. On
   success, returns subprocess' pid; on error, returns 0. */

int runcmd (const char *command, int *result, const int *io) /* ToDO: const char* */
{
    int pid, status;
    int aux, i, tmp_result;
    char *args[RCMD_MAXARGS], *p, *cmd;

    tmp_result = 0;

    /* Parse arguments to obtain an argv vector. */

    cmd = malloc ((strlen (command)+1) * sizeof(char));
    /* TODO sysfail (!cmd, -1); */
    p = strcpy (cmd, command);

    i=0;
    args[i++] = strtok (cmd, RCMD_DELIM);
    while ((i<RCMD_MAXARGS) && (args[i++] = strtok (NULL, RCMD_DELIM)));
    i--;

    if(!strcmp(args[i-1], "&")) {
        /* TODO */
        tmp_result |= NONBLOCK;
    }

    setup_signal_handlers();

    /* Create a subprocess. */
    pid = fork();

    if(pid < 0) {
        tmp_result |= EXECFAILSTATUS;
        pid = -1;
    }

    else if (pid>0) {		/* Caller process (parent). */
        if(!IS_NONBLOCK(tmp_result)) {
            aux = wait (&status);
            /* TODO sysfail (aux<0, -1); */
            if(aux < 0) {
            }

            /* Collect termination mode. */
            if (WIFEXITED(status)) {
                if(WEXITSTATUS(status) == 0) {
                    tmp_result |= EXECFAILSTATUS;
                } else {
                    tmp_result |= WEXITSTATUS(status);
                    tmp_result |= NORMTERM;
                    tmp_result |= EXECOK;
                }
            }
        }
    }

    else {			/* Subprocess (child) */
        if(IS_NONBLOCK(tmp_result)) {
            pid = fork();
            if(pid < 0) {
                exit(EXECFAILSTATUS);
            } else if(pid > 0) {
                aux = waitpid(pid, &status, 0);
                exit(0);
            }
        }

        if(io != NULL) {
            if(io[0] != 0)
                dup2(io[0], 0);
            if(io[1] != 1)
                dup2(io[1], 1);
            if(io[2] != 2)
                dup2(io[2], STDERR_FILENO);
        }

        aux = execvp (args[0], args);
        free (cmd);

        if(aux < 0)
            exit (0);

        exit (EXECFAILSTATUS);
    }

    if (result)
        *result = tmp_result;

    free (p);
    return pid;			/* Only parent reaches this point. */
}
