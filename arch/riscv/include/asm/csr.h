/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* party copied from the Linux kernel sources */

#include <grinch/const.h>
#include <grinch/stringify.h>

/* Definition for CSR register numbers, that are not available for RISC-V
 * binutils < 2.38
 */
#define CSR_SATP	0x180
#define CSR_VSSTATUS	0x200
#define CSR_VSIE	0x204
#define CSR_VSTVEC	0x205
#define CSR_VSSCRATCH	0x240
#define CSR_VSEPC	0x241
#define CSR_VSCAUSE	0x242
#define CSR_VSTVAL	0x243
#define CSR_VSIP	0x244
#define CSR_VSATP	0x280
#define CSR_HSTATUS	0x600
#define CSR_HEDELEG	0x602
#define CSR_HIDELEG	0x603
#define CSR_HIE		0x604
#define CSR_HTIMEDELTA	0x605
#define CSR_HCOUNTEREN	0x606
#define CSR_HGEIE	0x607
#define CSR_HENVCFG	0x60a
#define CSR_HTVAL	0x643
#define CSR_HIP		0x644
#define CSR_HVIP	0x645
#define CSR_HTINST	0x64a
#define CSR_HGATP	0x680

/* Status register flags */
#define SR_SIE		_UL(0x00000002) /* Supervisor Interrupt Enable */
#define SR_MIE		_UL(0x00000008) /* Machine Interrupt Enable */
#define SR_SPIE		_UL(0x00000020) /* Previous Supervisor IE */
#define SR_MPIE		_UL(0x00000080) /* Previous Machine IE */
#define SR_SPP		_UL(0x00000100) /* Previously Supervisor */
#define SR_MPP          _UL(0x00001800) /* Previously Machine */
#define SR_SUM		_UL(0x00040000) /* Supervisor User Memory Access */

#define SR_FS		_UL(0x00006000) /* Floating-point Status */
#define SR_FS_OFF	_UL(0x00000000)
#define SR_FS_INITIAL	_UL(0x00002000)
#define SR_FS_CLEAN	_UL(0x00004000)
#define SR_FS_DIRTY	_UL(0x00006000)

#define SR_XS		_UL(0x00018000) /* Extension Status */
#define SR_XS_OFF	_UL(0x00000000)
#define SR_XS_INITIAL	_UL(0x00008000)
#define SR_XS_CLEAN	_UL(0x00010000)
#define SR_XS_DIRTY	_UL(0x00018000)

#define SR_SD		_UL(0x8000000000000000) /* FS/XS dirty */

/* Exception causes */
#define EXC_INST_MISALIGNED		0
#define EXC_INST_ACCESS			1
#define EXC_INST_ILLEGAL		2
#define EXC_BREAKPOINT			3
#define EXC_LOAD_ACCESS_MISALIGNED	4
#define EXC_LOAD_ACCESS			5
#define EXC_AMO_ADDRESS_MISALIGNED	6
#define EXC_STORE_ACCESS		7
#define EXC_SYSCALL			8
#define EXC_HYPERVISOR_SYSCALL		9
#define EXC_SUPERVISOR_SYSCALL		10
#define EXC_INST_PAGE_FAULT		12
#define EXC_LOAD_PAGE_FAULT		13
#define EXC_STORE_PAGE_FAULT		15
#define EXC_INST_GUEST_PAGE_FAULT	20
#define EXC_LOAD_GUEST_PAGE_FAULT	21
#define EXC_VIRTUAL_INST_FAULT		22
#define EXC_STORE_GUEST_PAGE_FAULT	23

/* Exception cause high bit - is an interrupt if set */
#define CAUSE_IRQ_FLAG	(_UL(1) << (__riscv_xlen - 1))

/* Interrupt causes (minus the high bit) */
#define IRQ_S_SOFT		1
#define IRQ_VS_SOFT		2
#define IRQ_M_SOFT		3
#define IRQ_S_TIMER		5
#define IRQ_VS_TIMER		6
#define IRQ_M_TIMER		7
#define IRQ_S_EXT		9
#define IRQ_VS_EXT		10
#define IRQ_M_EXT		11

#define VSIP_TO_HVIP_SHIFT	(IRQ_VS_SOFT - IRQ_S_SOFT)

#define HCOUNTEREN_CY		(1 << 0)
#define HCOUNTEREN_TM		(1 << 1)
#define HCOUNTEREN_IR		(1 << 2)

/* IE/IP (Supervisor/Machine Interrupt Enable/Pending) flags */
#define IE_SIE		(_UL(0x1) << IRQ_S_SOFT)
#define IE_TIE		(_UL(0x1) << IRQ_S_TIMER)
#define IE_EIE		(_UL(0x1) << IRQ_S_EXT)

#define VIE_SIE		(IE_SIE << VSIP_TO_HVIP_SHIFT)
#define VIE_TIE		(IE_TIE << VSIP_TO_HVIP_SHIFT)
#define VIE_EIE		(IE_EIE << VSIP_TO_HVIP_SHIFT)

/* SATP flags */
#define SATP_PPN	_UL(0x00000FFFFFFFFFFF)
#define SATP_MODE_39	_UL(0x8000000000000000)
#define SATP_MODE_48	_UL(0x9000000000000000)
#define SATP_ASID_BITS	16
#define SATP_ASID_SHIFT	44
#define SATP_ASID_MASK	_UL(0xFFFF)

/* HSTATUS flags */
#define HSTATUS_VSXL            _UL(0x300000000)
#define HSTATUS_VSXL_SHIFT      32

#define HSTATUS_VTSR            _UL(0x00400000)
#define HSTATUS_VTW             _UL(0x00200000)
#define HSTATUS_VTVM            _UL(0x00100000)
#define HSTATUS_VGEIN           _UL(0x0003f000)
#define HSTATUS_VGEIN_SHIFT     12
#define HSTATUS_HU              _UL(0x00000200)
#define HSTATUS_SPVP            _UL(0x00000100)
#define HSTATUS_SPV             _UL(0x00000080)
#define HSTATUS_GVA             _UL(0x00000040)
#define HSTATUS_VSBE            _UL(0x00000020)

#ifndef __ASSEMBLY__

#define csr_read(csr)                                           \
({                                                              \
	register unsigned long __v;                             \
	__asm__ __volatile__ ("csrr %0, " __stringify(csr)      \
			      : "=r" (__v) :                    \
			      : "memory");                      \
	__v;                                                    \
})

#define csr_write(csr, val)                                     \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrw " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#define csr_swap(csr, val) 	                                \
({                                                              \
	register unsigned long __r;                             \
	unsigned long __w = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrrw %0, " __stringify(csr) ", %1" \
			      : "=r" (__r) : "rK" (__w)         \
			      : "memory");                      \
	__r;							\
})

#define csr_clear(csr, val)                                     \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrc " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#define csr_set(csr, val)                                       \
({                                                              \
	unsigned long __v = (unsigned long)(val);               \
	__asm__ __volatile__ ("csrs " __stringify(csr) ", %0"   \
			      : : "rK" (__v)                    \
			      : "memory");                      \
})

#endif /* __ASSEMBLY__ */
