/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grinch/grinch.h>
#include <grinch/types.h>
#include <grinch/vm.h>
#include <grinch/vsprintf.h>

#include <sys/stat.h>
#include <sys/wait.h>

#define PROMPT	"gsh> "

#define ASCII_BACKSPACE	0x8
#define ASCII_TAB	0x9
#define ASCII_CR	'\r'
#define ASCII_DEL	0x7f

#define CMDLINE_BUF_GROWTH	32
#define CMDLINE_BUF_MAX		256

int main(int argc, char *argv[], char *envp[]);
APP_NAME(gsh);

struct tokens {
	char *tokenised;
	char **tokens;
};

struct gsh_builtin {
	const char *cmd;
	int (*fun)(char *argv[]);
};

static struct tokens paths;
static char **_envp;

static inline void putc(char c)
{
	write(stdout, &c, 1);
}

static inline void putc_triple(char a, char b, char c)
{
	char buf[3] = {a, b, c};
	write(stdout, &buf, 3);
}

static int read_line(char **_buf)
{
	char *buf, *buf_new, *pos, c;
	size_t remaining, buf_sz;
	ssize_t bread;
	int err;

	buf = malloc(CMDLINE_BUF_GROWTH);
	if (!buf)
		return -ENOMEM;
	remaining = buf_sz = CMDLINE_BUF_GROWTH;
	pos = buf;

	do {
		if (remaining == 0) {
			if (buf_sz == CMDLINE_BUF_MAX) {
				err = -E2BIG;
				break;
			}
			buf_new = realloc(buf, buf_sz + CMDLINE_BUF_GROWTH);
			if (!buf_new) {
				err = -ENOMEM;
				break;
			}
			buf = buf_new;
			remaining += CMDLINE_BUF_GROWTH;
			pos = buf + buf_sz;
			buf_sz += CMDLINE_BUF_GROWTH;
		}

		bread = read(stdin, &c, 1);
		if (bread == -1) {
			perror("read");
			err = -errno;
			break;
		} else if (bread == 0)
			continue;

		if (c == ASCII_TAB) {
			/* no action on tab atm */
			continue;
		} else if (c == ASCII_CR) {
			*pos = 0;
			err = 0;
			break;
		} else if (c == ASCII_DEL) {
			if (pos != buf) {
				pos--;
				remaining++;
				putc_triple(ASCII_BACKSPACE, ' ', ASCII_BACKSPACE);
			}
			continue;
		} else
			*pos = c;

		putc(c);
		pos++;
		remaining--;
	} while (1);

	if (err)
		free(buf);
	else
		*_buf = buf;

	return err;
}

static int start(const char *cmd, char *argv[])
{
	pid_t child;
	int wstatus;

	child = fork();
	if (child == 0) {
		execve(cmd, argv, _envp);
		perror("execve");
		return -errno;
	} else if (child == -1) {
		perror("fork");
		return -errno;
	}

	child = waitpid(child, &wstatus, 0);
	if (WIFEXITED(wstatus)) {
		printf("Exit code %u\n", WEXITSTATUS(wstatus));
	}
	return 0;
}

static int gsh_version(char *argv[])
{
	printf("Grinch Shell v0.1\n");

	return 0;
}

static int gsh_exit(char *argv[])
{
	return -EIO;
}

static int gsh_help(char *argv[])
{
	printf("No help at the moment\n");

	return 0;
}

static int gsh_ps(char *argv[])
{
	return grinch_kstat(GRINCH_KSTAT_PS, 0);
}

static int gsh_kstat(char *argv[])
{
	const char *arg;
	pid_t pid;

	arg = argv[1];
	if (!arg)
		return -EINVAL;

	if (!strcmp(arg, "ps"))
		return gsh_ps(NULL);
	if (!strcmp(arg, "kheap"))
		return grinch_kstat(GRINCH_KSTAT_KHEAP, 0);
	if (!strcmp(arg, "maps")) {
		if (argv[2] != 0)
			pid = strtoul(argv[2], NULL, 0);
		else
			pid = getpid();

		return grinch_kstat(GRINCH_KSTAT_MAPS, pid);
	} else
		return -ENOSYS;

	return 0;
}

static int gsh_vm(char *argv[])
{
	pid_t child;
	int err;

	child = create_grinch_vm();
	if (child == -1) {
		perror("create grinch vm");
		err = -errno;
	} else {
		printf("Grinch VM: %d\n", child);
		err = 0;
	}

	return err;
}

static const struct gsh_builtin builtins[] = {
	{ "help", gsh_help },
	{ "version", gsh_version },
	{ "exit", gsh_exit },
	{ "k", gsh_kstat },
	{ "kernel", gsh_kstat },
	{ "ps", gsh_ps },
	{ "vm", gsh_vm },
};

static char *executable_get_pathname(const char *cmd)
{
	char *buf, **token;
	struct stat st;
	int err;

	/* Do we have an absolute path? */
	if (*cmd == '/')
		return strdup(cmd);

	if (!paths.tokens)
		return NULL;

	buf = malloc(128);
	for (token = paths.tokens; *token; token++) {
		snprintf(buf, 128, "%s/%s", *token, cmd);

		err = stat(buf, &st);
		if (err)
			continue;

		return buf;
	}

	free(buf);

	return NULL;
}

static int parse_command(int argc, char *argv[])
{
	const struct gsh_builtin *builtin;
	char *executable;
	const char *cmd;
	unsigned int i;
	int err;

	cmd = argv[0];

	for (builtin = builtins, i = 0; i < ARRAY_SIZE(builtins);
	     builtin++, i++) {
		if (strcmp(cmd, builtin->cmd))
			continue;
		err = builtin->fun(argv);
		return err;
	}

	executable = executable_get_pathname(cmd);
	if (!executable)
		return -ENOENT;

	err = start(executable, argv);
	free(executable);

	return err;
}

static void free_tokens(struct tokens *t)
{
	if (t->tokenised) {
		free(t->tokenised);
		t->tokenised = NULL;
	}

	if (t->tokens) {
		free(t->tokens);
		t->tokens = NULL;
	}
}

static int parse_tokens(const char *input_buffer, char delim, struct tokens *t)
{
	char **tokens, *start, *pos, c, l;
	unsigned int no_tokens;
	bool dquote, squote;

	/* First run: calculate maximum amount of arguments */
	no_tokens = strcount(input_buffer, delim) + 1;
	t->tokenised = malloc(strlen(input_buffer) + 1);
	if (!t->tokenised)
		return -ENOMEM;
	t->tokens = malloc(sizeof(char **) * (no_tokens + 1));
	if (!t->tokens) {
		free_tokens(t);
		return -ENOMEM;
	}

	start = pos = t->tokenised;
	tokens = t->tokens;
	dquote = squote = false;
	no_tokens = 0;
	c = 0;
	do {
		l = c;
		c = *input_buffer++;
		if (c == '\0') {
			if (dquote || dquote)
				goto error;

			*pos++ = 0;
			*tokens++ = start;
			no_tokens++;
			break;
		}

		if (c == '"' && l != '\\') {
			if (squote)
				goto store;

			dquote ^= true;
		}

		if (c == '\'' && l != '\\') {
			if (dquote)
				goto store;

			squote ^= true;
		}

		if (!dquote && !squote && c == delim) {
			*pos++ = 0;
			*tokens++ = start;
			start = pos;
			no_tokens++;

			while (*input_buffer == delim)
				input_buffer++;

			continue;
		}

store:
		*pos++ = c;
	} while (true);

	*tokens = NULL;

	return no_tokens;

error:
	free_tokens(t);

	return -EINVAL;
}

/*
 * Trims the string str in-situ. Removes {pre,succ}eeding whitespaces (includes
 * tabs). The infix of the string will not be touched.
 */
static void trim(char *str)
{
	char *dst, *lastwhite;

	dst = str;
	while (isblank(*str))
		str++;

	lastwhite = NULL;
	while (*str) {
		if (isblank(*str)) {
			if (!lastwhite)
				lastwhite = dst;
		} else
			lastwhite = NULL;

		*dst++ = *str++;
	}

	if (lastwhite)
		*lastwhite = '\0';
}

static int gsh(void)
{
	struct tokens tokens = { 0 };
	char *input_buffer;
	int argc, err;

	input_buffer = NULL;
	for (;;) {
		if (input_buffer) {
			free(input_buffer);
			input_buffer = NULL;
		}
		free_tokens(&tokens);

		puts(PROMPT);

		err = read_line(&input_buffer);
		if (err) {
			printf("\nError: %pe\n", ERR_PTR(err));
			continue;
		}

		trim(input_buffer);

		argc = parse_tokens(input_buffer, ' ', &tokens);
		if (argc <= 0) {
			printf("\nError: parsing cmdline\n");
			continue;
		}

		// FIXME: Evaluate command line and expand variables

		putc('\n');
		if (!strlen(input_buffer))
			continue;

		err = parse_command(argc, tokens.tokens);
		if (err == -EIO)
			break;

		if (err) {
			printf("Error: %pe\n", ERR_PTR(err));
		}
	}

	return err;
}

int main(int argc, char *argv[], char *envp[])
{
	char *path;
	int err;

	_envp = envp;

	path = getenv("PATH");
	if (!path)
		printf("Warning: no PATH found\n");
	else
		parse_tokens(path, ':', &paths);

	err = gsh();

	free_tokens(&paths);

	return err;
}
