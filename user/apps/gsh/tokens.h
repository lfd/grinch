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

struct tokens {
	char **tokens;
	unsigned int elements;
};

int tokens_from_array(char ** array, struct tokens *t);
int tokens_from_string(const char *input_buffer, char delim, struct tokens *t);
int tokens_duplicate(struct tokens *dst, struct tokens *src);
int tokens_add(struct tokens *t, const char *src);
void tokens_free(struct tokens *t);
