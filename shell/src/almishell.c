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

        if(!strcmp(j.first_process->argv[0], "exit") || !strcmp(j.first_process->argv[0], "quit")) {
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
