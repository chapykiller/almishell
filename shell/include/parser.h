#ifndef PARSER_H
#define PARSER_H

#include <process.h>
#include <job.h>

size_t count_pipes(char *command_line);

struct process *parse_process(char *command);

struct process *parse_last_process(struct job *j, char *command);

char parse_last_ampersand(char *command_line);

struct job *parse_command_line(char *command_line);

#endif /* PARSER_H */
