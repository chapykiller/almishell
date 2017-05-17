#include <runcmd.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void (*runcmd_onexit)(void) = NULL;

size_t count_non_space_sequence(const char *str) {
	size_t count = 0, i = 0;

	if(!str)
		return 0;

	/* Process command argument */
	while(str[i] != '\0') {
		/* Ignore whitespace */
		if(isspace(str[i])) ++i;

		/* Handle POSIX's Portable Filename Character Set */
		else if(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/') {
			++i; ++count;
			while(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/')
				++i;
			break;
		}
	}

	while(str[i] != '\0') {
		/* Ignore whitespace sequence */
		if(isspace(str[i])) ++i;

		/* Handle string argument, can contain anything between quotes */
		else {
			++count;
			if(str[i] == '\"' && (i == 0 || str[i-1] != '\\')) {
				++i;
				while(str[i] != '\0') {
					if(str[i] == '\"' && str[i-1] != '\\')
					{ ++i; break; }
					++i;
				}
			}
			else while(!isspace(str[i]) && str[i] != '\0') ++i;
	  }
	}

	return count;
}

char **extract_program_name_and_arguments(size_t names_count, const char *str, char **name_and_args) {
	size_t seq_begin, seq_size, i = 0, j = 0;

	/* The user should allocate the buffer */
	if(!name_and_args)
		return NULL;

	/* Process command argument */
	while(str[i] != '\0') {
		/* Ignore whitespace */
		if(isspace(str[i])) ++i;

		/* Handle POSIX's Portable Filename Character Set */
		else if(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/') {
			seq_begin = i; ++i;
			while(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/')
				++i;
			seq_size = i - seq_begin;
			name_and_args[j] = (char *) malloc(sizeof(char) * (seq_size + 1));
			strncpy(name_and_args[j], &str[seq_begin], seq_size);
			name_and_args[j][seq_size] = '\0';
			++j;
			break;
		}
	}

	while(str[i] != '\0') {
		/* Ignore whitespace sequence */
		if(isspace(str[i])) ++i;

		else if(str[i] != '\0') {
			seq_begin = i;

			/* Handle string argument, can contain anything between quotes */
			if(str[i] == '\"' && (i == 0 || str[i-1] != '\\')) {
				++i;
				while(str[i] != '\0') {
					if(str[i] == '\"' && str[i-1] != '\\')
					{ ++i; break; }
					++i;
				}
			}
			/* Any argument */
			else while(str[i] != '\0' && !isspace(str[i])) ++i;

			seq_size = i - seq_begin;
			name_and_args[j] = (char *) malloc(sizeof(char) * (seq_size + 1));
			strncpy(name_and_args[j], &str[seq_begin], seq_size);
			name_and_args[j][seq_size] = '\0';
			++j;
	  }
	}

	name_and_args[j] = (char *) malloc(sizeof(char));
	name_and_args[j][0] = '\0';

	return name_and_args;
}

int runcmd(const char *command, int *result, const int io[3]) {
	/* Extract program name and arguments */
	size_t names_count = count_non_space_sequence(command);
	char **name_and_args = (char**) malloc(sizeof(char *) * (names_count + 1));
	/* PID for storing fork() calls */
	pid_t pid;
	int exit_status;

	extract_program_name_and_arguments(names_count, command, name_and_args);

	pid = fork();
	if(pid == 0) {
		execv(name_and_args[0], &name_and_args[1]);
		free(name_and_args);
		exit(EXIT_FAILURE);
	}
	else if(pid > 0){
      /*if(BLOCKING_MODE_FLAG)*/
			waitpid(pid, &exit_status, 0);
	}

	if(result)
		*result = WEXITSTATUS(exit_status);

	free(name_and_args);
	return pid;
}
