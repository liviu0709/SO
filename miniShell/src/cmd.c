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

#define READ 0
#define WRITE 1
#define ERROR 2
#define COPY_WRITE 1023
#define COPY_READ 1022
#define COPY_ERR 1021

static int init;

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	if (dir == NULL)
		return 1;
	if (chdir(dir->string)) {
		fprintf(stderr, "no such file or directory\n");
		return 1;
	}
	return 0;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	return SHELL_EXIT;
}

void repairFds(bool redirectedOutput, bool redirectedInput, bool redirectedError, bool redirectedOutAndErr)
{
	if (redirectedOutput) {
		close(WRITE);
		dup2(COPY_WRITE, WRITE);
	}
	if (redirectedInput) {
		close(READ);
		dup2(COPY_READ, READ);
	}
	if (redirectedOutAndErr)
		return;
	if (redirectedError) {
		close(ERROR);
		dup2(COPY_ERR, ERROR);
	}
}

char *combineParts(word_t *word)
{
	char *result = malloc(strlen(word->string) + 1);\

	if (word->expand) {
		if (getenv(word->string) == NULL)
			setenv(word->string, "", 1);
		strcpy(result, getenv(word->string));
	} else {
		strcpy(result, word->string);
	}
	word_t *current = word->next_part;

	while (current) {
		if (current->expand) {
			if (getenv(current->string) == NULL)
				setenv(current->string, "", 1);
			result = realloc(result, strlen(result) + strlen(getenv(current->string)) + 1);
			strcat(result, getenv(current->string));
		} else {
			result = realloc(result, strlen(result) + strlen(current->string) + 1);
			strcat(result, current->string);
		}
		current = current->next_part;
	}
	return result;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	if (strcmp(s->verb->string, "exit") == 0 || strcmp(s->verb->string, "quit") == 0)
		return shell_exit();

	bool redirectedOutput = false;
	bool redirectedInput = false;
	bool redirectedError = false;
	bool redirectedOutAndErr = false;
	char *outString = NULL;
	if (s->out) {
		int fd;
		char *path = combineParts(s->out);

		outString = path;
		if (s->io_flags & IO_OUT_APPEND)
			fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
		else
			fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		dup2(fd, WRITE);
		redirectedOutput = true;
	}
	if (s->in) {
		char *path = combineParts(s->in);
		int fd = open(path, O_RDONLY);

		dup2(fd, READ);
		redirectedInput = true;
	}
	if (s->err) {
		char *path = combineParts(s->err);
		bool handled = false;

		if (redirectedOutput) {
			if (strcmp(path, outString) == 0) {
				dup2(WRITE, ERROR);
				redirectedOutAndErr = true;
				handled = true;
			}
		}
		if (!handled) {
			int fd;

			if (s->io_flags & IO_ERR_APPEND)
				fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
			else
				fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			dup2(fd, ERROR);
			redirectedError = true;
		}
	}

	if (strcmp(s->verb->string, "cd") == 0) {
		int ret = shell_cd(s->params);

		repairFds(redirectedOutput, redirectedInput, redirectedError, redirectedOutAndErr);
		return ret;
	}

	if (s->verb->next_part) {
		if (s->verb->next_part->string[0] == '=') {
			char *value = combineParts(s->verb->next_part->next_part);
			int ret = setenv(s->verb->string, value, 1);

			repairFds(redirectedOutput, redirectedInput, redirectedError, redirectedOutAndErr);
			return ret;
		}
	}

	int argCount = 0;
	char **argList = get_argv(s, &argCount);
	pid_t pid = fork();

	if (pid == 0) {
		execvp(s->verb->string, argList);
		printf("Execution failed for '%s'\n", s->verb->string);
		exit(1);
	} else {
		int status;

		waitpid(pid, &status, 0);
		repairFds(redirectedOutput, redirectedInput, redirectedError, redirectedOutAndErr);
		return status;
	}
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
							command_t *father)
{
	int pid = fork();

	if (pid == 0) {
		parse_command(cmd1, level + 1, father);
		exit(0);
	} else {
		return parse_command(cmd2, level + 1, father);
	}
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
						command_t *father)
{
	int fdin = dup(READ);
	int fdout = dup(WRITE);
	int pipeArray[2], ret;
	pipe(pipeArray);

	pid_t pid = fork();

	if (pid == 0) {
		close(pipeArray[0]);
		dup2(pipeArray[1], WRITE);
		ret = parse_command(cmd1, level + 1, father);
		close(pipeArray[1]);
		exit(ret);
	} else {
		close(pipeArray[1]);
		dup2(fdout, WRITE);
		dup2(pipeArray[0], READ);
		ret = parse_command(cmd2, level + 1, father);
		close(pipeArray[0]);
		dup2(fdin, READ);
	}
	close(fdin);
	close(fdout);
	return ret;
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	int ret;
	if (init == 0) {
		dup2(WRITE, COPY_WRITE);
		dup2(READ, COPY_READ);
		dup2(ERROR, COPY_ERR);
		init = 1;
	}

	switch (c->op) {
	case OP_NONE:
		ret = parse_simple(c->scmd, level, father);
		break;
	case OP_SEQUENTIAL:
		parse_command(c->cmd1, level + 1, c);
		ret = parse_command(c->cmd2, level + 1, c);
		break;

	case OP_PARALLEL:
		ret = run_in_parallel(c->cmd1, c->cmd2, level + 1, c);
		break;

	case OP_CONDITIONAL_NZERO:
		ret = parse_command(c->cmd1, level + 1, c);
		if (ret != 0)
			ret = parse_command(c->cmd2, level + 1, c);
		break;

	case OP_CONDITIONAL_ZERO:
		ret = parse_command(c->cmd1, level + 1, c);
		if (ret == 0)
			ret = parse_command(c->cmd2, level + 1, c);
		break;

	case OP_PIPE:
		ret = run_on_pipe(c->cmd1, c->cmd2, level + 1, c);
		break;

	default:
		return SHELL_EXIT;
	}

	return ret;
}
