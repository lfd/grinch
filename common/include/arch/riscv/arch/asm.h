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

#if ARCH_RISCV == 64 /* rv64 */
#define REG_S	sd /* 64-Bit Double Load */
#define REG_L	ld /* 32-Bit Double Load */
#define SZREG	8 /* Register Size: 8 Byte / 64 Bit */
#else
#error "Unknown RISC-V Architecture"
#endif
