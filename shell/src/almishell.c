#include <process.h>
#include <job.h>
#include <shell.h>
#include <parser.h>

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

void print_prompt(const char *path)
{
    const char *prompt_str = "$ ";

    printf("[%s]%s", path, prompt_str);
    fflush(stdout);
}

/* Caller must free the allocated memory */
char *read_command_line(FILE *input)
{
    char *command_line = NULL;
    size_t buffer_size = 0;

    ssize_t command_line_size = getline(&command_line, &buffer_size, input);

    /* If there's at least one character other than newline */
    if(command_line_size > 1) {
        command_line[--command_line_size] = '\0'; /* Remove the newline char */
        return command_line;
    }

    free(command_line);

    /* If end of file is reached or CTRL + D is received, the command is exit */
    if(feof(input)) {
        clearerr(input);
        printf("\n");
        fflush(stdout);
        command_line = (char *) malloc(sizeof(char) * (strlen(shell_cmd[SHELL_EXIT]) + 1));
        strcpy(command_line, shell_cmd[SHELL_EXIT]);
        return command_line;
    }

    if(command_line_size == -1)
        perror("almishell: getline");

    return NULL;
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
    FILE *input = stdin;

    struct shell_info shinfo = init_shell();
    struct job *tail_job, *current_job, *previous_job;

    shinfo.first_job = tail_job = NULL;

    if(argc > 1) {
        if(strcmp(argv[1], "--command") == 0 || strcmp(argv[1], "-c") == 0) {
            if(argc >= 3) {
                command_line = extract_command_line(argc, argv);
                shinfo.run = 0; /* Run shell a single time */
            } else {
                printf("almishell: %s: requires an argument\n", argv[1]);
                return EXIT_FAILURE;
            }
        } else if(strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("Almishell, version 1.0.0\n");
            return EXIT_FAILURE;
        } else if(argv[1][0] == '-') {
            printf("almishell: %s: invalid option\n", argv[1]);
            return EXIT_FAILURE;
        } else if(argc > 2) {
            printf("%s", "almishell: too many arguments");
            return EXIT_FAILURE;
        } else {
            input = fopen(argv[1], "r");
            if(!input) {
                perror("almishell: fopen");
                return EXIT_FAILURE;
            }
        }
    }

    do {
        struct job *j;

        while(!command_line) {
            if(input == stdin)
                print_prompt(shinfo.current_path);
            command_line = read_command_line(input);
        }

        j = parse_command_line(command_line);

        if(check_processes(j)) {
            if(!j) {
                printf("almishell: syntax error\n");
            } else {
                j->id = tail_job ? tail_job->id+1 : 1;

                if(!shinfo.first_job) {
                    shinfo.first_job = tail_job = j;
                    shinfo.minusJob = shinfo.plusJob = 1;
                } else {
                    tail_job->next = j;
                    tail_job = j;

                    shinfo.minusJob = shinfo.plusJob;
                    shinfo.plusJob = j->id;
                }

                launch_job(&shinfo, j, shinfo.first_job);
            }
        } else {
            if(j->first_process && j->first_process->next)
                printf("almishell: syntax error\n");
        }

        current_job = shinfo.first_job;
        previous_job = NULL;

        while(current_job) {
            if(job_is_completed(current_job)) {
                if(current_job->id == shinfo.plusJob) {
                    shinfo.plusJob = shinfo.minusJob;
                } else if(current_job->id == shinfo.minusJob) {
                    shinfo.minusJob = shinfo.plusJob;
                }
                if(shinfo.first_job == tail_job) {
                    delete_job(current_job);
                    current_job = shinfo.first_job = tail_job = NULL;
                    shinfo.plusJob = shinfo.minusJob = 0;
                } else if(current_job == shinfo.first_job) {
                    shinfo.first_job = current_job->next;
                    delete_job(current_job);
                    current_job = shinfo.first_job;
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

    if(input != stdin)
        fclose(input);

    return EXIT_SUCCESS;
}
