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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <grinch/grinch.h>
#include <grinch/types.h>
#include <grinch/vm.h>
#include <grinch/vsprintf.h>

#include <sys/wait.h>

#define PROMPT	"gsh> "

#define ASCII_BACKSPACE	0x8
#define ASCII_TAB	0x9
#define ASCII_CR	'\r'
#define ASCII_DEL	0x7f

int main(int argc, char *argv[], char *envp[]);

static char **_envp;

APP_NAME(gsh);

struct gsh_builtin {
	const char *cmd;
	int (*fun)(char *argv[]);
};

static inline void putc(char c)
{
	write(stdout, &c, 1);
}

static inline void putc_triple(char a, char b, char c)
{
	char buf[3] = {a, b, c};
	write(stdout, &buf, 3);
}

static int read_line(char *buf, size_t len)
{
	ssize_t bread;
	char c, *pos;

	pos = buf;
	do {
		if (len == 0)
			return -E2BIG;

		bread = read(stdin, &c, 1);
		if (bread == -1) {
			perror("read");
			return -errno;
		} else if (bread == 0)
			continue;

		if (c == ASCII_TAB) {
			/* no action on tab atm */
			continue;
		} else if (c == ASCII_CR) {
			*pos = 0;
			break;
		} else if (c == ASCII_DEL) {
			if (pos != buf) {
				pos--;
				len++;
				putc_triple(ASCII_BACKSPACE, ' ', ASCII_BACKSPACE);
			}
			continue;
		} else
			*pos = c;

		putc(c);
		pos++;
		len--;
	} while (1);

	return 0;
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
	return grinch_ps();
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
	{ "ps", gsh_ps },
	{ "vm", gsh_vm },
};

static int parse_command(int argc, char *argv[])
{
	const struct gsh_builtin *builtin;
	const char *cmd;
	unsigned int i;
	char buf[64];
	int err;

	cmd = argv[0];

	for (builtin = builtins, i = 0; i < ARRAY_SIZE(builtins);
	     builtin++, i++) {
		if (strcmp(cmd, builtin->cmd))
			continue;
		err = builtin->fun(argv);
		return err;
	}

	snprintf(buf, sizeof(buf), "/initrd/%s", cmd);
	err = start(buf, argv);

	return err;
}

static int parse_tokens(const char *input_buffer, char *token_buffer, char *tokens[])
{
	unsigned int no_tokens;
	char *start;
	char c;
	//char quote;
	//bool is_escape;

	//is_escape = false;
	//quote = 0;
	start = token_buffer;
	no_tokens = 0;
	do {
		c = *input_buffer++;
		if (c == '\0') {
			*token_buffer++ = 0;
			*tokens++ = start;
			no_tokens++;
			break;
		}

		if (c == ' ') {
			*token_buffer++ = 0;
			*tokens++ = start;
			start = token_buffer;
			no_tokens++;
			continue;
		}

		*token_buffer++ = c;
	} while (true);

	*tokens = NULL;

	return no_tokens;
}

static int gsh(void)
{
	char input_buffer[32];
	char token_buffer[64];
	char *argv[32];
	int argc, err;

	printf_set_prefix(false);
	for (;;) {
		puts(PROMPT);

		err = read_line(input_buffer, sizeof(input_buffer));
		if (err) {
			printf("\nError: %pe\n", ERR_PTR(err));
			continue;
		}

		argc = parse_tokens(input_buffer, token_buffer, argv);
		if (argc <= 0) {
			printf("\nError: parsing cmdline\n");
			continue;
		}

		putc('\n');
		if (!strlen(input_buffer))
			continue;

		err = parse_command(argc, argv);
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
	int err;

	_envp = envp;
	err = gsh();

	return err;
}
