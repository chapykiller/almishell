#include <runcmd.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void (*runcmd_onexit)(void) = NULL;

size_t count_non_space_sequence(const char *str) {
	size_t count = 0, i = 0;

	if(!str)
		return 0;

	while(str[i] != '\0') {
		// Ignore whitespace sequence
		if(isspace(str[i])) ++i;

		// Handle string argument, can contain anything between quotes
		else if(str[i] == '\"' && (i == 0 || str[i-1] != '\\')) {
			++i; ++count;
			while(str[i] != '\0') {
				if(str[i] == '\"' && (i == 0 || str[i-1] != '\\'))
				{ ++i; break; }
				++i;
			}
		}

		// Handle POSIX's Portable Filename Character Set
		else if(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/') {
			++i; ++count;
			while(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/')
				++i;
		}
	}

	return count;
}

char **extract_program_name_and_arguments(size_t names_count, const char *command, char **name_and_args) {
	// The user should allocate the buffer
	if(!name_and_args)
		return NULL;

	size_t seq_begin, seq_size, j = 0;
	while(str[i] != '\0') {
		// Ignore whitespace sequence
		if(isspace(str[i])) ++i;

		// Handle string argument, can contain anything between quotes
		else if(str[i] == '\"' && (i == 0 || str[i-1] != '\\')) {
			seq_begin = i; ++i;
			while(str[i] != '\0') {
				if(str[i] == '\"' && (i == 0 || str[i-1] != '\\'))
				{ ++i; break; }
				++i;
			}
			name_and_args[j] = (char *) malloc(sizeof(char) * (i - seq_begin));
			++j;
		}

		// Handle POSIX's Portable Filename Character Set
		else if(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/') {
			seq_begin = i; ++i;
			while(isalnum(str[i]) || str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '/')
				++i;

			seq_size = i - seq_begin;
			name_and_args[j] = (char *) malloc(sizeof(char) * (seq_size + 1));
			strncpy(&name_and_args[j], &str[seq_begin], seq_size);
			name_and_args[j][seq_size] = '\0';
			++j;
		}
	}

	name_and_args[j + 1] = '\0';

	return name_and_args;
}

int runcmd(const char *command, int *result, const int[3] io) {
	// Extract program name and arguments
	size_t names_count = count_non_space_sequence(command);
	char **name_and_args = (char**) malloc(sizeof(char *) * (names_count + 1));
	extract_program_name_and_arguments(names_count, command, name_and_args);

	// TODO: After fork call exec like this:
	execv(name_and_args[0], &name_and_args[1]);
}
