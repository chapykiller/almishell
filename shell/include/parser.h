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
