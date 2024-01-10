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

#include <generated/compile.h>
#include <generated/version.h>

#define VERSION_STRING 	__stringify(VERSION) "." __stringify(PATCHLEVEL) \
			__stringify(EXTRAVERSION)

#define UNAME_S		"Grinch"
#define UNAME_R		VERSION_STRING

#define UNAME_SHORT	UNAME_S " version " UNAME_R

#define UNAME_A		UNAME_SHORT				\
			" (" COMPILE_BY "@" COMPILE_HOST ")"	\
			" (" COMPILE_CC_VERSION ")"		\
			" " COMPILE_DATE
