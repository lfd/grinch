/* SPDX-License-Identifier: GPL-2.0 */

/* Copied from the Linux kernel sources */

#ifndef _GCONFIG_H
#define _GCONFIG_H

/*
 * Whether a configuration symbol is set, as a plain 0 or 1. Asking this way
 * keeps both arms of the condition compiled, so the one that is off cannot
 * rot unseen. It needs every name it mentions to exist in either case.
 */
#define __ARG_PLACEHOLDER_1		0,
#define __take_second_arg(__ignored, val, ...)	val

#define __is_defined(x)			___is_defined(x)
#define ___is_defined(val)		____is_defined(__ARG_PLACEHOLDER_##val)
#define ____is_defined(arg1_or_junk)	__take_second_arg(arg1_or_junk 1, 0)

#define IS_ENABLED(option)		__is_defined(option)

#endif /* _GCONFIG_H */
