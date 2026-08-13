/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ASM_CLINT_H
#define _ASM_CLINT_H

#include <grinch/mmio.h>
#include <grinch/types.h>

#include <asm/counter.h>
#include <asm/csr.h>

/*
 * The timer and software interrupts a supervisor kernel asks firmware for are
 * these registers. Addressed by hart id, taken from the CSR rather than from
 * the per-CPU data, which is not up yet when the first hart arrives here.
 */
#define CLINT_MSIP		0x0000
#define CLINT_MTIMECMP		0x4000
#define CLINT_MTIME		0xbff8

extern void *clint;

int clint_init(void);

static inline u64 clint_time(void)
{
#ifdef CONFIG_ARCH_RISCV64
	return mmio_read64(clint + CLINT_MTIME);
#else
	return read_counter64(mmio_read32(clint + CLINT_MTIME + 4),
			      mmio_read32(clint + CLINT_MTIME));
#endif
}

static inline void clint_timer_set(u64 ticks)
{
	void *cmp = clint + CLINT_MTIMECMP + csr_read(mhartid) * sizeof(u64);

#ifdef CONFIG_ARCH_RISCV64
	mmio_write64(cmp, ticks);
#else
	/* No 64 bit store: park the compare beyond reach while it is torn. */
	mmio_write32(cmp + 4, -1);
	mmio_write32(cmp, ticks);
	mmio_write32(cmp + 4, ticks >> 32);
#endif
}

static inline void clint_ipi(unsigned long hart, bool pending)
{
	mmio_write32(clint + CLINT_MSIP + hart * sizeof(u32), pending);
}

static inline void clint_ipi_clear(void)
{
	clint_ipi(csr_read(mhartid), false);
}

#endif /* _ASM_CLINT_H */
