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

void handle_signal(int signal) {
}

int main(int argc, char *argv[])
{
    const char *prompt_str = "$ ";
    char *command_line = NULL;
    ssize_t command_line_size = 0;
    size_t buffer_size = 0;

    struct shell_info shinfo = init_shell();
    struct job *first_job, **j, *current_job, *previous_job;
    struct sigaction sact;

    int run = 1; /* Control the shell main loop */
    int job_id = 1;

    /* Setup the sighub handler */
    sact.sa_handler = &handle_signal;

    /* Intercept SIGINT */
    if (sigaction(SIGINT, &sact, NULL) == -1) {
        perror("Error: cannot handle SIGINT\n"); /* Should not happen */
    }
    /* Intercept SIGQUIT */
    if (sigaction(SIGQUIT, &sact, NULL) == -1) {
        perror("Error: cannot handle SIGQUIT\n"); /* Should not happen */
    }
    /* Intercept SIGTSTP */
    if (sigaction(SIGTSTP, &sact, NULL) == -1) {
        perror("Error: cannot handle SIGTSTP\n"); /* Should not happen */
    }

    j = &first_job;

    while(run) {
        do {
            if(command_line) {
                free(command_line);
            }
            command_line = NULL;
            buffer_size = 0;

            printf("[%s]%s", shinfo.current_path, prompt_str);
            fflush(stdout);
            command_line_size = getline(&command_line, &buffer_size, stdin);

            /* Close the shell when receive End of Transmission */
            if(command_line_size < 0) {
                int isEOT = !errno;

                clearerr(stdin);
                printf("\n");
                fflush(stdout);

                if(isEOT) {
                    free(command_line);

                    command_line_size = strlen(shinfo.cmd[0]) + 2;
                    command_line = (char*) malloc(command_line_size * sizeof(char));

                    strcpy(command_line, shinfo.cmd[0]);
                }
            }
        } while(command_line_size < 2);

        command_line[--command_line_size] = '\0';
        /* TODO: Handle failure */

        *j = parse_command_line(command_line);
        (*j)->id = job_id++;

        if(check_processes(*j)) {
            int it;

            if(!*j) {
                printf("Syntax Error\n"); /* TODO: Improve error message */
            }
            else {
                for(it = 0; it < shinfo.num_cmd; ++it) {
                    if(!(*j)->first_process->next
                        && !strcmp((*j)->first_process->p->argv[0], shinfo.cmd[it])) {
                        switch (it) {
                            case 0: /* exit */
                            case 1: /* quit */
                                run = 0;
                                break;
                            case 2: /* cd */
                                if((*j)->first_process->p->argv[1]) {
                                    chdir((*j)->first_process->p->argv[1]);

                                    free(shinfo.current_path);
                                    shinfo.current_path = getcwd(NULL, 0);
                                }
                                break;
                            case 3: /* jobs */
                                break;
                            case 4: /* fg */
                                break;
                            case 5: /* bg */
                                break;
                            default:
                                break;
                        }

                        it = shinfo.num_cmd + 1;
                    }
                }

                if(it <= shinfo.num_cmd) {
                    launch_job(&shinfo, *j, first_job, 1);
                }

                j = &((*j)->next);
            }

            /* TODO: Handle job list, some running in the background and others not */
        }

        current_job = first_job;
        previous_job = NULL;
        while(current_job) {
            struct job *temp;

            temp = current_job;
            current_job = current_job->next;

            if(job_is_completed(temp)) {
                if(temp == first_job)
                    first_job = first_job->next;

                if(previous_job) {
                    previous_job->next = temp->next;
                }

                delete_job(temp);
                temp = previous_job;
            }

            previous_job = temp;
        }

        free(command_line);
        command_line = NULL;
        buffer_size = 0;
    }

    delete_shell(&shinfo);

    return EXIT_SUCCESS;
}
