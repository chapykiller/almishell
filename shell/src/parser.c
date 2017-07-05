/*
ALMiSHELL - A POSIX conformant shell prototype.
Copyright (C) 2017  Henrique C. S. M. Aranha; Lucas E. C. Mello; Lucas H. F. Leal

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <parser.h>

#include <limits.h>
#include <fcntl.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
