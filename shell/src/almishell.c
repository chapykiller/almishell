#include <sys/types.h>
#include <sys/wait.h>

#include <limits.h>
#include <termios.h>
#include <unistd.h>

/* Structure representing a process, from glibc manual*/
struct process {
    char argv[_POSIX_ARG_MAX];  /* for exec */
    pid_t pid;                  /* process ID */
    char completed;             /* true if process has completed */
    char stopped;               /* true if process has stopped */
    int status;                 /* reported status value */
    int io[3];
};

/* Preliminary job representation, for a job with a single process */
struct job {
    struct process *first_process;
}

struct shell_info {
    int terminal;
    int interactive;
    pid_t pgid;
    struct termios tmodes;
};

struct process init_process(const char *command_line)
{
    struct process p;

    p.io[0] = STDIN_FILENO;
    p.io[1] = STDOUT_FILENO;
    p.io[2] = STDERR_FILENO;

    p.completed = 0;
    p.stopped = 0;
    command_line
}

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
            kill(-pgid, SIGTTIN);

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
        tcgetattr(info.terminal, &info.tmodes)
    }

    return info;
}

/* NOTE: Should be called after fork */
void run_process(struct shell_info *s, struct process *p, int fg)
{
    pid_t pid;
    const std_filenos[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

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
    for(int i = 0; i < 3; ++i) {
        if(p.io[i] != std_filenos[i]) {
            dup2(p.io[i], std_filenos[i]);
            close(p.io[i]);
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

    /* TODO: Handler pipes */
    if(!s->interactive)
        waitpid(pid, &ret_status, 0);
    else if(fg)
        put_job_in_foreground(j, 0);
    /* TODO: Handle background */

}

int main(int argc, char *argv[])
{
    const char *prompt_str = "$ ";
    char command_line[_POSIX_ARG_MAX];

    struct shell_info shinfo = init_shell();
    struct job j;

    int run = 1;
    while(run) {
        printf("%s", prompt_str);
        fflush(stdout);

        /* TODO: Break on error */
        read_command_line(command_line);

        j.first_process = (struct process *) malloc(sizeof(struct process));
        parse_command_line(command_line, j.first_process);
        /* TODO: Handle invalid command line */

        run_job(&shinfo, j, 1);

        free(j.first_process);
    }

    return EXIT_SUCCESS;
}
