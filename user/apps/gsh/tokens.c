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
#include <stdlib.h>
#include <string.h>

#include "tokens.h"

void tokens_free(struct tokens *t)
{
	char **tokens;

	if (!t || !t->tokens)
		return;

	tokens = t->tokens;
	for (tokens = t->tokens; *tokens; tokens++)
		free(*tokens);

	t->tokens = NULL;
	t->elements = 0;
}

static inline char **malloc_array(unsigned int elements)
{
	return malloc(sizeof(char **) * (elements + 1));
}

int tokens_duplicate(struct tokens *dst, struct tokens *src)
{
	char **array;

	array = malloc_array(src->elements);
	if (!array)
		return -ENOMEM;

	dst->tokens = array;
	dst->elements = 0;

	for (; dst->elements < src->elements; dst->elements++) {
		dst->tokens[dst->elements] = strdup(src->tokens[dst->elements]);
		if (!dst->tokens[dst->elements]) {
			tokens_free(dst);
			return -ENOMEM;
		}
	}

	dst->tokens[dst->elements] = NULL;

	return 0;
}

int tokens_add(struct tokens *t, const char *src)
{
	char **array, *new;

	new = strdup(src);
	if (!new)
		return -ENOMEM;

	array = malloc_array(t->elements + 1);
	if (!array) {
		free(new);
		return -ENOMEM;
	}

	memcpy(array, t->tokens, t->elements * sizeof(*t->tokens));
	array[t->elements] = new;
	array[t->elements + 1] = NULL;
	free(t->tokens);
	t->tokens = array;
	t->elements++;

	return 0;
}

int tokens_from_array(char **array, struct tokens *t)
{
	unsigned int elements;
	char **tmp;

	for (elements = 0, tmp = array; *tmp; tmp++, elements++);

	t->tokens = malloc_array(elements);
	if (!t->tokens)
		return -ENOMEM;
	t->elements = elements;

	for (tmp = t->tokens; *array; array++, tmp++) {
		*tmp = strdup(*array);
		if (!*tmp) {
			tokens_free(t);
			return -ENOMEM;
		}
	}

	return 0;
}

static char *strdup_diff(const char *start, const char *end)
{
	ptrdiff_t len;
	char *dst;

	len = end - start;
	dst = malloc(len + 1);
	if (!dst)
		return NULL;

	memcpy(dst, start, len);
	dst[len] = 0;

	return dst;
}

int tokens_from_string(const char *input_buffer, char delim, struct tokens *t)
{
	char **tokens, *new, c, l;
	const char *start, *end;
	unsigned int elements;
	bool dquote, squote;

	/* First run: calculate maximum amount of arguments */
	elements = strcount(input_buffer, delim) + 1;
	t->tokens = malloc_array(elements);
	if (!t->tokens) {
		tokens_free(t);
		return -ENOMEM;
	}

	start = end = input_buffer;
	tokens = t->tokens;
	dquote = squote = false;
	elements = 0;
	c = 0;
	do {
		l = c;
		c = *input_buffer++;
		if (c == '\0') {
			if (dquote || dquote)
				goto error;

			goto end_token;
		}

		if (c == '"' && l != '\\') {
			if (squote)
				goto next;

			dquote ^= true;
		}

		if (c == '\'' && l != '\\') {
			if (dquote)
				goto next;

			squote ^= true;
		}

		if (!dquote && !squote && c == delim) {
			goto end_token;
		}

next:
		end++;
		continue;

end_token:
		new = strdup_diff(start, end);
		*tokens++ = new;
		elements++;

		if (c == '\0')
			break;

		while (*input_buffer == delim)
			input_buffer++;

		start = end = input_buffer;
	} while (true);

	*tokens = NULL;
	t->elements = elements;

	return elements;

error:
	tokens_free(t);

	return -EINVAL;
}


