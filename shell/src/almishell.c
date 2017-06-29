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
    int argc = 0, i, p_argc;

    args[argc++] = strtok(command, command_delim);
    if(!args[0]) {
        free(p);
        return NULL;
    }

    while( (argc < _POSIX_ARG_MAX) && (args[argc++] = strtok(NULL, command_delim)) );

    p->argv = (char **) malloc(argc-- * sizeof(char *));

    for(p_argc = 0, i = 0; args[i]; ++i) {
        p->argv[p_argc] = (char*) malloc((strlen(args[i]) + 1) * sizeof(char));
        strcpy(p->argv[p_argc++], args[i]);
    }

    p->argv[p_argc] = (char*)NULL;

    return p;
}

struct process *parse_last_process(struct job *j, char *command)
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

    for(p_argc = 0, i = 0; args[i]; ++i) {
        int true_arg = 1;

        if(i + 1 != argc) {
            if(args[i][0] == '<') {
                fd = open(args[++i], O_RDONLY);

                if(fd < 0)
                    perror("almishell: open");
                else
                    j->io[0] = fd;

                true_arg = 0;
            } else if(args[i][0] == '>') {
                fd = open(args[++i], O_WRONLY|O_CREAT, 0666);

                if(fd < 0)
                    perror("almishell: open");
                else
                    j->io[1] = fd;

                true_arg = 0;
            }
        }

        if(true_arg) {
            p->argv[p_argc] = (char*) malloc((strlen(args[i]) + 1) * sizeof(char));
            strcpy(p->argv[p_argc++], args[i]);
        }
    }

    p->argv[p_argc] = (char*)NULL;

    return p;
}

char parse_last_ampersand(char *command_line)
{
    size_t i = 1;
    char *ampersand = strrchr(command_line, '&');

    /* If & is not found, there's no need to remove it */
    if(!ampersand)
        return 'f';

    /* Skip spaces */
    while(isspace(ampersand[i])) ++i;

    /* If there were only spaces after the ampersand, remove it and the space */
    if(ampersand[i] == '\0') {
        ampersand[0] = '\0';
        return 'b'; /* Signal background */
    } else if(ampersand[i] == '<' || ampersand[i] == '>') { /* If the & is followed by file redirection */
        ampersand[0] = ' ';
        return 'b'; /* Signal background */
    }

    return 'f'; /* Signal foreground */
}

struct job *parse_command_line(char *command_line)
{
    size_t i = 0, command_num = count_pipes(command_line) + 1;
    const char *command_delim = "|";
    char **commands = (char **) malloc(sizeof(char *) * command_num);
    struct job *j;
    struct process_node **next, *current;
    char background = parse_last_ampersand(command_line);

    j = init_job(command_line, background);
    next = &j->first_process;

    commands[i++] = strtok(command_line, command_delim);
    while( (i < command_num) && (commands[i++] = strtok(NULL, command_delim)) );

    i = 0;
    while( (i + 1 < command_num) && commands[i] ) {
        current = (struct process_node *) malloc(sizeof(struct process_node));
        current->p = parse_process(commands[i]);
        current->next = NULL;

        *next = current;
        next = &current->next;

        ++i;
    }

    /* Handle last process */
    current = (struct process_node *) malloc(sizeof(struct process_node));
    current->p = parse_last_process(j, commands[i]);
    current->next = NULL;
    *next = current;

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

    if(command_line_size < 0)
        perror("almishell: getline");

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

    case 'a':
        if(strcmp(shell_cmd[SHELL_ALMISHELL], cmd) == 0)
            return SHELL_ALMISHELL;
        break;

    default:
        break;
    }

    return SHELL_NONE;
}

void run_jobs(struct job *first_job)
{
    struct job *it = first_job;

    update_status(first_job);

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
        } else if(job_is_stopped(it)) {
            printf("Stopped");
            /* TODO: Identify the signal that made it stop, like in bash */
        } else { /* Job is running */
            printf("Running");
        }

        printf("\t\t\t%s%s\n", it->command, it->background == 'b' ? " &" : "");
        it = it->next;
    }
}

void fg_bg(struct shell_info *sh, struct job *first_job, char **args, int id)
{
    int i;
    struct job *current;
    struct process_node *node_p;

    if(!args[1]) {
        i = plusJob;
    } else {
        i = atoi(args[1]);
    }

    if(i < 1) {
        printf("almishell: fg: %s: no such job\n", args[1] ? args[1] : "current");
        return;
    }

    current = first_job;
    while(current) {
        if(current->id == i) {
            if(plusJob != i)
                minusJob = plusJob;
            plusJob = i;

            printf("%s\n", current->command);

            for (node_p = current->first_process; node_p; node_p = node_p->next)
                node_p->p->stopped = 0;

            if(id == SHELL_FG)
                put_job_in_foreground(sh, current, first_job, 1);
            else
                put_job_in_background(sh, current, first_job, 1);

            return;
        }
    }


    printf("almishell: fg: %s: no such job\n", args[1] ? args[1] : "current");
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
                perror("almishell: cd");

            free(sh->current_path);
            sh->current_path = getcwd(NULL, 0);
        }
        break;

    case SHELL_JOBS:
        run_jobs(first_job);
        break;

    case SHELL_FG:
    case SHELL_BG:
        fg_bg(sh, first_job, args, id);
        break;

    case SHELL_ALMISHELL:
        printf("\"Os alunos tão latindo Michel, traz a antirábica.\"\n");
        fflush(stdout);
        break;

    default:
        printf("almishell: invalid command\n");
        fflush(stdout);
        break;
    }
}

/* Extract command line from shell arguments on -c mode */
char *extract_command_line(int argc, char *argv[])
{
    int i;
    char *command_line = NULL;
    size_t command_line_size = 0;

    for(i = 2; i < argc; ++i)
        command_line_size += strlen(argv[i]) + 1; /* arg + separator char size */

    command_line = (char *) malloc(sizeof(char) * command_line_size);

    for(i = 2; i < argc; ++i) {
        strcat(command_line, argv[i]);
        strcat(command_line, " ");
    }

    command_line[command_line_size - 1] = '\0';

    return command_line;
}

int main(int argc, char *argv[])
{
    char *command_line = NULL;

    struct shell_info shinfo = init_shell();
    struct job *first_job, *tail_job, *current_job, *previous_job;

    first_job = tail_job = NULL;

    if(argc > 1) {
        if(strcmp(argv[1], "--command") == 0 || strcmp(argv[1], "-c") == 0) {
            if(argc >= 3) {
                command_line = extract_command_line(argc, argv);
                shinfo.run = 0; /* Run shell a single time */
            } else {
                printf("almishell: %s: requires an argument\n", argv[1]);
                return 0;
            }
        } else if(strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("Almishell, version 1.0.0\n");
            return 0;
        } else {
            printf("almishell: %s: invalid option\n", argv[1]);
            return 0;
        }
    }

    do {
        struct job *j;

        while(!command_line) {
            print_prompt(shinfo.current_path);
            command_line = read_command_line();
        }

        j = parse_command_line(command_line);

        if(check_processes(j)) {
            if(!j) {
                printf("almishell: syntax error\n");
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

                    launch_job(&shinfo, j, first_job);
                } else { /* Built-in command */
                    run_builtin_command(&shinfo, first_job, j->first_process->p->argv, cmd);
                    delete_job(j);
                }
            }
        } else {
            if(j->first_process && j->first_process->next)
                printf("almishell: syntax error\n");
        }

        current_job = first_job;
        previous_job = NULL;

        while(current_job) {
            if(job_is_completed(current_job)) {
                if(current_job->id == plusJob) {
                    plusJob = minusJob;
                } else if(current_job->id == minusJob) {
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
    } while(shinfo.run);

    delete_shell(&shinfo);

    return EXIT_SUCCESS;
}
