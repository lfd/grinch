/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grinch/reboot_abi.h>

int main(int argc, char **argv);

static void print_usage(const char *program_name)
{
	printf("Usage: %s [-r | --reboot] [-h | --halt]\n", program_name);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--halt")) {
		printf("Halting...\n");
		return reboot(GRINCH_REBOOT_CMD_HALT);
	}

	if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "--reboot")) {
		printf("Rebooting...\n");
		return reboot(GRINCH_REBOOT_CMD_REBOOT);
	}

	print_usage(argv[0]);
	return EXIT_FAILURE;
}
