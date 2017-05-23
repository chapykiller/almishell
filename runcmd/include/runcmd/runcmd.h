#ifndef RUNCMD_H
#define RUNCMD_H

/* Definitions for the command line parser. */

#define RCMD_MAXARGS   1024	/* Max number of arguments. */
#define RCMD_DELIM    " \n\t\r" /* Command line delimiters.   */
#define RCMD_NONBLOCK '&'	/* Command line nonblock sign.*/

/* Bit mask for interpreting command execution results. */

#define NORMTERM    (1 <<  8)	/* 256 */
#define EXECOK      (1 <<  9)	/* 1024 */
#define NONBLOCK    (1 << 10)	/* 2048 */
#define RETSTATUS   (0xFF)	/* 255 */

/*
 *   This is the user API.
 */

/* Macros to obtain information on command execution results
   reported by 'runcmd' call.

   IS_NORMTERM(result) returns true if the subprocess has terminated
                       normally; false otherwise.
   IS_NONBLOCK(result) returns true if the subprocess has been executed
                       in nonblocking mode; false otherwise.
   IS_EXECOK(result)   returns true if 'command' has been effectively
		       executed; false otherwise.
                       the subprocess; false otherwise.
   EXITSTATUS(result)  returns the exit status returned by the
                       subproccess.

*/

#define IS_NORMTERM(result) ((result & NORMTERM) && 1)
#define IS_NONBLOCK(result) ((result & NONBLOCK) && 1)
#define EXITSTATUS(result)  ( result & RETSTATUS)
#define IS_EXECOK(result)   ((result & EXECOK) && 1)

/* Subprocess' exit status upon exec failure.*/

#define EXECFAILSTATUS 127

/* Run a command in a subprocess. */

int runcmd (const char *command, int *result, const int *io);

/* Hanlder for SIGCHLD in nonblock mode. */

extern void (*runcmd_onexit)(void);

#endif	/* RUNCMD_H */
