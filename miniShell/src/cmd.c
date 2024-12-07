// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"
#include "utils.h"

#define READ		0
#define WRITE		1
#define ERROR		2
#define COPY_WRITE 1023
#define COPY_READ 1022
#define COPY_ERR 1021

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	/* TODO: Execute cd. */

	return 0;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	/* TODO: Execute exit/quit. */

	return SHELL_EXIT; /* TODO: Replace with actual exit code. */
}
char* addPath(char* path, const char* command) {
    if (command[0] == '/' | command[0] == '.') {
        return command;
    }
    char* result = malloc(strlen(path) + strlen(command) + 2);
    strcpy(result, path);
    strcat(result, "/");
    strcat(result, command);
    return result;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	/* TODO: Sanity checks. */
    bool redirectedOutput = false;
    bool redirectedInput = false;
    bool redirectedError = false;
    bool redirectedOutAndErr = false;
    if (s->out) {
        int fd;
        // For now theres just one possible output file
        if (s->io_flags & IO_OUT_APPEND) {
            fd = open(s->out->string, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else {
            fd = open(s->out->string, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        dup2(WRITE, COPY_WRITE);
        dup2(fd, WRITE);
        redirectedOutput = true;
    }
    if (s->in) {
        int fd = open(s->in->string, O_RDONLY);
        dup2(READ, COPY_READ);
        dup2(fd, READ);
        redirectedInput = true;
    }
    if (s->err) {
        bool handled = false;
        if ( redirectedOutput ) {
            if (strcmp(s->err->string, s->out->string) == 0) {
                dup2(WRITE, ERROR);
                redirectedOutAndErr = true;
                handled = true;
            }
        }
        if (!handled) {
            int fd;
            if (s->io_flags & IO_ERR_APPEND) {
                fd = open(s->err->string, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } else {
                fd = open(s->err->string, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            dup2(ERROR, COPY_ERR);
            dup2(fd, ERROR);
            redirectedError = true;
        }
    }

	/* TODO: If variable assignment, execute the assignment and return
	 * the exit status.
	 */

    if (strcmp(s->verb->string, "exit") == 0 || strcmp(s->verb->string, "quit") == 0) {
                return shell_exit();
        }
    int argCount = 0;
    char** argList = get_argv(s, &argCount);
    pid_t pid = fork();
    if (pid == 0) {
        char* path = addPath("/bin", s->verb->string);
        execv(path, argList);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (redirectedOutput) {
            close(WRITE);
            dup2(COPY_WRITE, WRITE);
        }
        if (redirectedInput) {
            close(READ);
            dup2(COPY_READ, READ);
        }
        if ( redirectedOutAndErr )
            return status;
        if (redirectedError) {
            close(ERROR);
            dup2(COPY_ERR, ERROR);
        }
        return status;
    }
	return 0; /* TODO: Replace with actual exit status. */
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Execute cmd1 and cmd2 simultaneously. */

	return true; /* TODO: Replace with actual exit status. */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Redirect the output of cmd1 to the input of cmd2. */

	return true; /* TODO: Replace with actual exit status. */
}


/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	/* TODO: sanity checks */

	if (c->op == OP_NONE) {
        return parse_simple(c->scmd, level, father);


	}

	switch (c->op) {
	case OP_SEQUENTIAL:
		/* TODO: Execute the commands one after the other. */
		break;

	case OP_PARALLEL:
		/* TODO: Execute the commands simultaneously. */
		break;

	case OP_CONDITIONAL_NZERO:
		/* TODO: Execute the second command only if the first one
		 * returns non zero.
		 */
		break;

	case OP_CONDITIONAL_ZERO:
		/* TODO: Execute the second command only if the first one
		 * returns zero.
		 */
		break;

	case OP_PIPE:
		/* TODO: Redirect the output of the first command to the
		 * input of the second.
		 */
		break;

	default:
		return SHELL_EXIT;
	}

	return 0; /* TODO: Replace with actual exit code of command. */
}
