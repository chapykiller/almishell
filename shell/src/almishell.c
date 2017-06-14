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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

size_t count_pipes(char *command_line)
{
    size_t pipe_num = 0;
    char *pipe_pos = strchr(command_line, '|');

    if(!pipe_pos)
        return 0;

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
            p->argv[p_argc++] = args[i];
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
    struct job *j = init_job();
    struct process_node **next = &j->first_process, *current;

    commands[0] = strtok(command_line, command_delim);
    while( (i < command_num) && commands[i] ) {
        if(i + 1 == command_num && strchr(commands[i], '&'))
            j->background = 'b';

        current = (struct process_node *) malloc(sizeof(struct process_node));
        current->p = parse_process(commands[i]);
        current->next = NULL;

        *next = current;
        next = &current->next;

        if(++i < command_num)
            commands[i] = strtok(NULL, command_delim);
    }

    free(commands);

    return j;
}

int main(int argc, char *argv[])
{
    const char *prompt_str = "$ ";
    char *command_line = NULL;
    ssize_t command_line_size = 0;
    size_t buffer_size = 0;

    struct shell_info shinfo = init_shell();
    struct job *j;

    int run = 1; /* Control the shell main loop */

    while(run) {
        do {
            if(command_line) {
                free(command_line);
                command_line = NULL;
                buffer_size = 0;
            }

            printf("%s", prompt_str);
            fflush(stdout);
            command_line_size = getline(&command_line, &buffer_size, stdin);
        } while(command_line_size < 2);

        command_line[--command_line_size] = '\0';
        /* TODO: Handle failure */

        j = parse_command_line(command_line);
        /* TODO: Handle invalid command line */

        if(!strcmp(j->first_process->p->argv[0], "exit")
           || !strcmp(j->first_process->p->argv[0], "quit")) {
            run = 0;
        } else {
            launch_job(&shinfo, j, 1);
        }

        /* TODO: Handle job list, some running in the background and others not */
        delete_job(j);

        free(command_line);
        command_line = NULL;
        buffer_size = 0;
    }

    return EXIT_SUCCESS;
}
