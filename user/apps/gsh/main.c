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

#include <sys/stat.h>
#include <sys/wait.h>

#include <grinch/grinch.h>
#include <grinch/types.h>
#include <grinch/vm.h>
#include <grinch/vsprintf.h>

#include "tokens.h"

#define NAME	"gsh"
#define PROMPT	NAME "> "

#define ASCII_BACKSPACE	0x8
#define ASCII_TAB	0x9
#define ASCII_CR	'\r'
#define ASCII_DEL	0x7f

#define GSH_BUF_GROWTH	32
#define GSH_BUF_MAX	256

int main(int argc, char *argv[], char *envp[]);
APP_NAME(gsh);

struct gsh_builtin {
	const char *cmd;
	int (*fun)(char *argv[]);
};

struct gcall {
	const char *name;
	unsigned int no;
};

const struct gcall gcalls[] = {
	{"kheap", GCALL_KHEAP},
	{"ps", GCALL_PS},
	{"lsdev", GCALL_LSDEV},
	{"lspci", GCALL_LSPCI},
	{"lsof", GCALL_LSOF},
	{"maps", GCALL_MAPS},
};

static struct tokens paths;
static struct tokens orig_env;

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

	buf = malloc(GSH_BUF_GROWTH);
	if (!buf)
		return -ENOMEM;
	remaining = buf_sz = GSH_BUF_GROWTH;
	pos = buf;

	do {
		if (remaining == 0) {
			if (buf_sz == GSH_BUF_MAX) {
				err = -E2BIG;
				break;
			}
			buf_new = realloc(buf, buf_sz + GSH_BUF_GROWTH);
			if (!buf_new) {
				err = -ENOMEM;
				break;
			}
			buf = buf_new;
			remaining += GSH_BUF_GROWTH;
			pos = buf + buf_sz;
			buf_sz += GSH_BUF_GROWTH;
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

static int start(const char *cmd, char *argv[], char *env[])
{
	pid_t child;
	int wstatus;

	child = fork();
	if (child == 0) {
		execve(cmd, argv, env);
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
	return gcall(GCALL_PS, 0);
}

static int gsh_kstat(char *argv[])
{
	const struct gcall *gc;
	unsigned long arg;
	const char *cmd;
	unsigned int i;
	int err;

	cmd = argv[1];
	if (!cmd)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(gcalls); i++) {
		gc = gcalls + i;
		if (!strcmp(gc->name, cmd))
				goto call;
	}

	return -ENOSYS;

call:
	if (gc->no == GCALL_MAPS) {
		if (argv[2] != 0)
			arg = strtoul(argv[2], NULL, 0);
		else
			arg = getpid();
	} else {
		arg = 0;
	}

	err = gcall(gc->no, arg);

	if (err == -1)
		return -errno;

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

static int gsh_cd(char *argv[])
{
	char *dst;
	int err;

	if (!argv[1])
		return 0;

	dst=argv[1];

	err = chdir(dst);
	if (err == -1)
		return -errno;

	return 0;
}

static const struct gsh_builtin builtins[] = {
	{ "cd", gsh_cd },
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

	if (*cmd == '/' || *cmd == '.')
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

static int parse_command(int argc, char *argv[], char *env[])
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
	if (!executable) {
		printf(NAME ": Command not found\n");
		return -ENOENT;
	}

	err = start(executable, argv, env);
	free(executable);

	return err;
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

static int print_prompt(void)
{
	char *cwd;

	cwd = grinch_getcwd();
	if (!cwd)
		return -ENOMEM;

	printf("gsh %s> ", cwd);
	free(cwd);

	return 0;
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
		tokens_free(&tokens);

		err = print_prompt();
		if (err) {
			printf("\nError: %pe\n", ERR_PTR(err));
			break;
		}

		err = read_line(&input_buffer);
		if (err) {
			printf("\nError: %pe\n", ERR_PTR(err));
			continue;
		}

		trim(input_buffer);

		argc = tokens_from_string(input_buffer, ' ', &tokens);
		if (argc <= 0) {
			printf("\nError: parsing cmdline\n");
			continue;
		}

		// FIXME: Evaluate command line and expand variables

		putc('\n');
		if (!strlen(input_buffer))
			continue;

		err = parse_command(argc, tokens.tokens, orig_env.tokens);
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

	err = tokens_from_array(envp, &orig_env);
	if (err < 0)
		return -1;
	
	path = getenv("PATH");
	if (!path)
		printf("Warning: no PATH found\n");
	else
		tokens_from_string(path, ':', &paths);

	err = gsh();

	tokens_free(&paths);

	return err;
}
