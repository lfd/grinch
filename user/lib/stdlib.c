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

#include <stdlib.h>
#include <string.h>

char **__envp;

char *getenv(const char *name)
{
	char **envp;
	size_t len;

	len = strlen(name);

	for (envp = __envp; *envp; envp++)
		if (!strncmp(name, *envp, len))
			goto found;

	return NULL;

found:
	if (*(*envp + len) != '=')
		return NULL;

	return *envp + len + 1;
}
