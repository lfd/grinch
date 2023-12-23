/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/types.h>

#define SBI_EXT_0_1_CONSOLE_PUTCHAR	0x1

#define SBI_EXT_BASE			0x10
#define SBI_EXT_BASE_GET_SPEC_VERSION	0
#define SBI_EXT_BASE_GET_IMP_ID		1
#define SBI_EXT_BASE_GET_IMP_VERSION	2
#define SBI_EXT_BASE_PROBE_EXT		3
#define SBI_EXT_BASE_GET_MVENDORID	4
#define SBI_EXT_BASE_GET_MARCHID	5
#define SBI_EXT_BASE_GET_MIMPID		6

#define SBI_EXT_TIME			0x54494D45
#define SBI_EXT_TIME_SET_TIMER		0

#define SBI_EXT_IPI			0x735049
#define SBI_EXT_IPI_SEND_IPI		0

#define SBI_EXT_HSM			0x48534D
#define SBI_EXT_HSM_HART_START		0

/* "Grinch" SBI Extension */
#define SBI_EXT_GRNC			0x47524E48

#define SBI_SUCCESS		0
#define SBI_ERR_FAILED		-1
#define SBI_ERR_NOT_SUPPORTED	-2

struct sbiret {
	long error;
	long value;
};

static inline struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
				      unsigned long arg1, unsigned long arg2,
				      unsigned long arg3, unsigned long arg4,
				      unsigned long arg5)
{
	struct sbiret ret;

	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
	asm volatile ("ecall"
		    : "+r" (a0), "+r" (a1)
		    : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		    : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

static inline void sbi_console_putchar(int ch)
{
	sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

static inline struct sbiret __sbi_set_timer_v02(u64 stime_value)
{
	return sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, stime_value,
			 0, 0, 0, 0, 0);
}


static inline struct sbiret __sbi_send_ipi_v02(unsigned long hmask,
					       unsigned long hbase)
{
	return sbi_ecall(SBI_EXT_IPI, SBI_EXT_IPI_SEND_IPI, hmask, hbase, 0, 0, 0, 0);
}

static inline struct sbiret sbi_send_ipi(unsigned long hmask,
					 unsigned long hbase)
{
	return __sbi_send_ipi_v02(hmask, hbase);
}

static inline struct sbiret sbi_set_timer(u64 stime_value)
{
	return __sbi_set_timer_v02(stime_value);
}

static inline struct sbiret sbi_hart_start(unsigned long hartid,
					   unsigned long start_addr,
					   unsigned long opaque)
{
	return sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START, hartid,
			 start_addr, opaque, 0, 0, 0);
}

int sbi_init(void);
