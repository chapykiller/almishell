#include <process.h>
#include <job.h>
#include <shell.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <readline/readline.h>

int plusJob;
int minusJob;

size_t count_pipes(char *command_line)
{
    size_t pipe_num = 0;
    char *pipe_pos = strchr(command_line, '|');

    if(!pipe_pos)
        return 0;

    ++pipe_num;

    while( (pipe_pos = strchr(&pipe_pos[1], '|')) )
        ++pipe_num;

    return pipe_num;
}

struct process *parse_process(char *command)
{
    const char *command_delim = "\t ";
    struct process *p = init_process();
    char *args[_POSIX_ARG_MAX];
    int argc = 0, i, p_argc, fd;

    args[argc++] = strtok(command, command_delim);
    if(!args[0]) {
        free(p);
        return NULL;
    }

    while( (argc < _POSIX_ARG_MAX) && (args[argc++] = strtok(NULL, command_delim)) );

    p->argv = (char **) malloc(argc-- * sizeof(char *));

    /* TODO: Create a parse ignoring < and > for jobs with multiple processes
    and read those only in the end of the command line ? */
    for(p_argc = 0, i = 0; args[i]; ++i) {
        if(args[i][0] == '<') {
            if(i + 1 == argc)
                continue;

            fd = open(args[++i], O_RDONLY);
            if(fd < 0)
                perror("open");
            else
                p->io[0] = fd;
        } else if(args[i][0] == '>') {
            if(i + 1 == argc)
                continue;

            fd = open(args[++i], O_WRONLY|O_CREAT, 0666);
            if(fd < 0)
                perror("open");
            else
                p->io[1] = fd;
        } else if(args[i][0] != '&' || args[i][1] != '\0') {
            p->argv[p_argc] = (char*) malloc((strlen(args[i]) + 1) * sizeof(char));
            strcpy(p->argv[p_argc++], args[i]);
        }
    }

    p->argv[p_argc] = (char*)NULL;

    return p;
}

struct job *parse_command_line(char *command_line)
{
    size_t i = 0, command_num = count_pipes(command_line) + 1;
    const char *command_delim = "|";
    char **commands = (char **) malloc(sizeof(char *) * command_num);
    struct job *j = init_job(command_line);
    struct process_node **next = &j->first_process, *current;
    commands[i++] = strtok(command_line, command_delim);
    while( (i < command_num) && (commands[i++] = strtok(NULL, command_delim)) );

    i = 0;
    while( (i < command_num) && commands[i] ) {
        if(i + 1 == command_num && strchr(commands[i], '&'))
            j->background = 'b';

        current = (struct process_node *) malloc(sizeof(struct process_node));
        current->p = parse_process(commands[i]);
        current->next = NULL;

        *next = current;
        next = &current->next;

        ++i;
    }

    j->size = command_num;

    if(i < command_num && !commands[i]) {
        delete_job(j);
        j = NULL;
    }

    free(commands);

    return j;
}

void print_prompt(const char *path)
{
    const char *prompt_str = "$ ";

    printf("[%s]%s", path, prompt_str);
    fflush(stdout);
}

/* Caller must free the allocated memory */
char *read_command_line()
{
    char *command_line = NULL;
    size_t buffer_size = 0;

    ssize_t command_line_size = getline(&command_line, &buffer_size, stdin);

    /* If there's at least one character other than newline */
    if(command_line_size > 1) {
        command_line[--command_line_size] = '\0'; /* Remove the newline char */
        return command_line;
    }

    free(command_line);

    /* If end of file is reached or CTRL + D is received, the command is exit */
    if(feof(stdin)) {
        clearerr(stdin);
        printf("\n");
        fflush(stdout);
        command_line = (char *) malloc(sizeof(char) * (strlen(shell_cmd[SHELL_EXIT]) + 1));
        strcpy(command_line, shell_cmd[SHELL_EXIT]);
        return command_line;
    }

    /* TODO: Handle error */
    if(command_line_size < 0)
        perror("getline");

    return NULL;
}

/*  If it's a builtin command, returns its index in the shell_cmd array,
   else returns -1.
*/

enum SHELL_CMD is_builtin_command(struct job *j)
{
    const char *cmd = j->first_process->p->argv[0];

    if(j->size > 1)
        return SHELL_NONE;

    switch(cmd[0]) {
    case 'e':
        if(strcmp(shell_cmd[SHELL_EXIT], cmd) == 0)
            return SHELL_EXIT;
        break;

    case 'q':
        if(strcmp(shell_cmd[SHELL_QUIT], cmd) == 0)
            return SHELL_QUIT;
        break;

    case 'c':
        if(strcmp(shell_cmd[SHELL_CD], cmd) == 0)
            return SHELL_CD;
        break;

    case 'j':
        if(strcmp(shell_cmd[SHELL_JOBS], cmd) == 0)
            return SHELL_JOBS;
        break;

    case 'f':
        if(strcmp(shell_cmd[SHELL_FG], cmd) == 0)
            return SHELL_FG;
        break;

    case 'b':
        if(strcmp(shell_cmd[SHELL_BG], cmd) == 0)
            return SHELL_BG;
        break;

    default:
        break;
    }

    return SHELL_NONE;
}

void run_jobs(struct job *first_job) {
    struct job *it = first_job;

    while(it) {
        printf("[%d]%c  ", it->id, (plusJob==it->id ? '+' : (minusJob==it->id ? '-' : ' ')));

        if(job_is_completed(it)) {
            struct process_node *last = it->first_process;

            /* Loop until variable last holds the last process */
            while(last->next != NULL)
                last = last->next;

            printf("Done");

            if(last->p->status != 0)
                printf("(%d)", last->p->status);
        }
        else if(job_is_stopped(it)) {
            printf("Stopped");
            /* TODO: Identify the signal that made it stop, like in bash */
        }
        else { /* Job is running */
            printf("Running");
        }

        printf("\t\t\t%s\n", it->command);
        it = it->next;
    }
}

void fg_cmd(struct shell_info *sh, struct job *first_job, char **args) {
    int i;
    struct job *current;

    if(!args[1]) {
        i = plusJob;
    }
    else {
        i = atoi(args[1]);
    }

    current = first_job;
    while(current) {
        if(current->id == i) {
            if(plusJob != i)
                minusJob = plusJob;
            plusJob = i;

            printf("%s\n", current->command);
            put_job_in_foreground(sh, current, first_job, 1);
            return;
        }
    }
}

void run_builtin_command(struct shell_info *sh, struct job *first_job, char **args, int id)
{
    switch(id) {
    case SHELL_EXIT:
    case SHELL_QUIT:
        sh->run = 0;
        break;

    case SHELL_CD:
        if(args[1]) {
            if(chdir(args[1]) < 0)
                perror("almishell");

            free(sh->current_path);
            sh->current_path = getcwd(NULL, 0);
        }
        break;

    case SHELL_JOBS:
        run_jobs(first_job);
        break;

    case SHELL_FG:
        fg_cmd(sh, first_job, args);
        break;

    case SHELL_BG: /* TODO */
        break;

    default: /* TODO: Handle error */
        break;
    }
}

int main(int argc, char *argv[])
{
    char *command_line = NULL;

    struct shell_info shinfo = init_shell();
    struct job *first_job, *tail_job, *current_job, *previous_job;

    first_job = tail_job = NULL;

    while(shinfo.run) {
        struct job *j;

        do {
            print_prompt(shinfo.current_path);
            command_line = read_command_line();
        } while(!command_line);

        j = parse_command_line(command_line);

        if(check_processes(j)) {
            if(!j) {
                printf("Syntax Error\n"); /* TODO: Improve error message */
            } else {
                enum SHELL_CMD cmd = is_builtin_command(j);

                j->id = tail_job ? tail_job->id+1 : 1;

                if(cmd == SHELL_NONE) {
                    if(!first_job) {
                        first_job = tail_job = j;
                        minusJob = plusJob = 1;
                    } else {
                        tail_job->next = j;
                        tail_job = j;

                        minusJob = plusJob;
                        plusJob = j->id;
                    }

                    launch_job(&shinfo, j, first_job, 1);
                } else { /* Built-in command */
                    run_builtin_command(&shinfo, first_job, j->first_process->p->argv, cmd);
                    delete_job(j);
                }
            }

            /* TODO: Handle job list, some running in the background and others not */
        }

        current_job = first_job;
        previous_job = NULL;

        while(current_job) {
            if(job_is_completed(current_job)) {
                if(current_job->id == plusJob) {
                    plusJob = minusJob;
                }
                else if(current_job->id == minusJob) {
                    minusJob = plusJob;
                }
                if(first_job == tail_job) {
                    delete_job(current_job);
                    current_job = first_job = tail_job = NULL;
                    plusJob = minusJob = 0;
                } else if(current_job == first_job) {
                    first_job = current_job->next;
                    delete_job(current_job);
                    current_job = first_job;
                } else if(current_job == tail_job) {
                    tail_job = previous_job;

                    delete_job(current_job);

                    previous_job->next = NULL;

                    current_job = NULL;
                } else {
                    previous_job->next = current_job->next;
                    delete_job(current_job);
                }
            }
            previous_job = current_job;
            if(current_job)
                current_job = current_job->next;
        }


        if(command_line) {
            free(command_line);
            command_line = NULL;
        }
    }

    delete_shell(&shinfo);

    return EXIT_SUCCESS;
}
