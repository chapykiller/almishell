#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <limits.h>
#include <termios.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Structure representing a process, from glibc manual*/
struct process {
    char **argv;                /* for exec */
    pid_t pid;                  /* process ID */
    char completed;             /* true if process has completed */
    char stopped;               /* true if process has stopped */
    int status;                 /* reported status value */
    int io[3];
};

/* Preliminary job representation, for a job with a single process */
struct job {
    struct process *first_process;
};

struct shell_info {
    int terminal;
    int interactive;
    pid_t pgid;
    struct termios tmodes;
};

/* Ensures proper shell initialization, making sure the shell is executed in
foreground, based on
http://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html#Initializing-the-Shell */

struct shell_info init_shell()
{
    struct shell_info info;

    info.terminal = STDIN_FILENO;
    info.interactive = isatty(info.terminal);

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

/* NOTE: Should be called after fork */
void run_process(struct shell_info *s, struct process *p, int fg)
{
    int i;
    pid_t pid;
    const int std_filenos[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

    if(s->interactive) {
        pid = getpid();

        /* Create own process group */
        setpgid(pid, pid);

        /* If it should run on the foreground */
        if(fg)
            tcsetpgrp(s->terminal, pid);

        /* Set the handling for job control signals back to the default. */
        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    }

    /* Set the standard input/output channels of the new process.  */
    for(i = 0; i < 3; ++i) {
        if(p->io[i] != std_filenos[i]) {
            if(dup2(p->io[i], std_filenos[i]) < 0) {
                perror("dup2");
            }
            close(p->io[i]);
        }
    }

    execvp(p->argv[0], p->argv);

    /* TODO: Verify if execution state should be reported similarly to runcmd */
    perror("execvp");
    exit(EXIT_FAILURE);
}

void put_job_in_foreground(struct shell_info *s, struct job *j, int cont) {
    int ret_status;

    /* Give control access to the shell terminal to the job */
    tcsetpgrp(s->terminal, j->first_process->pid);

    /* TODO: If job is being continued */

    waitpid(j->first_process->pid, &ret_status, 0);

    /* Return access to the terminal to the shell */
    tcsetpgrp(s->terminal, s->pgid);

    /* Restore shell terminal modes */
    tcsetattr(s->terminal, TCSADRAIN, &s->tmodes);
}

void run_job(struct shell_info *s, struct job *j, int fg)
{
    struct process *p = j->first_process;
    pid_t pid;
    int ret_status;

    /* TODO: Extend for jobs with multiple processes */
    pid = fork();

    if(pid == 0) {
        run_process(s, p, fg);
    }
    else if(pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else {
        p->pid = pid;
        if(s->interactive) {
            /* TODO: Handle process group for whole jobs with multiple processes */
            setpgid(pid, pid);
        }
    }

    /* TODO: Handle pipes */
    if(!s->interactive)
        waitpid(pid, &ret_status, 0);
    else if(fg)
        put_job_in_foreground(s, j, 0);
    /* TODO: Handle background */

}

void parse_command_line(char *command_line, struct job *j) {
    const char *command_delim = "\t ";
    char **args = (char **) malloc(_POSIX_ARG_MAX * sizeof(char *));
    int argc = 0, i;
    int p_argc = 0;
    struct process *p = j->first_process;

    args[argc++] = strtok(command_line, command_delim);
    while( (argc < _POSIX_ARG_MAX) && (args[argc++] = strtok(NULL, command_delim)) );

    p->argv = (char **) malloc(argc * sizeof(char *));
    argc--;

    for(i = 0; args[i]; ++i) {
        if(args[i][0] == '<') {
            p->io[0] = open(args[i+1], O_RDONLY);
            ++i;
            if( p->io[0] < 0)
                perror("open");
            /* TODO: Handle open error */
            /* TODO: Handle failure, i+1 == argc and open return */
        }
        else if(args[i][0] == '>') {
            p->io[1] = open(args[i+1], O_WRONLY|O_CREAT, 0666);
            ++i;
            /* TODO: Handle failure, i+1 == argc and open return */
        }
        /* TODO: else if(args[i][0] == '&') */
        /* TODO: else if(args[i][0] == '|') */
        else {
            p->argv[p_argc++] = args[i];
        }
    }

    p->argv[p_argc] = (char*)NULL;

    free(args);
}

int main(int argc, char *argv[])
{
    const char *prompt_str = "$ ";
    char *command_line = NULL;
    ssize_t command_line_size = 0;
    size_t buffer_size = 0;

    struct shell_info shinfo = init_shell();
    struct job j;

    int run = 1; /* Control the shell main loop */

    j.first_process = (struct process *) malloc(sizeof(struct process));

    while(run) {
        j.first_process->io[0] = STDIN_FILENO;
        j.first_process->io[1] = STDOUT_FILENO;
        j.first_process->io[2] = STDERR_FILENO;

        do {
            printf("%s", prompt_str);
            fflush(stdout);
            command_line_size = getline(&command_line, &buffer_size, stdin);
        } while(command_line_size == 1);

        command_line[command_line_size-1] = '\0';
        /* TODO: Handle failure */

        parse_command_line(command_line, &j);
        /* TODO: Handle invalid command line */

        if(!strcmp(j.first_process->argv[0], "exit")) {
            run = 0;
        }
        else {
            run_job(&shinfo, &j, 1);
        }

        free(j.first_process->argv);
    }

    free(j.first_process);
    free(command_line);

    return EXIT_SUCCESS;
}
