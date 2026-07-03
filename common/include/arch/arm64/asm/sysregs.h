/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) OTH Regensburg, 2017
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* The following definitions are inspired by
 * hypervisor/arch/arm64/include/asm/sysregs.h */

#include <grinch/bits.h>
#include <grinch/stringify.h>

#define SCTLR_EL1_I	(1 << 12)
#define SCTLR_EL1_C	(1 << 2)
#define SCTLR_EL1_M	(1 << 0)

#define SCTLR		SCTLR_EL1

/* Enable MMU, data+instruction caches */
#define SCTLR_MMU_CACHES	(SCTLR_EL1_I | SCTLR_EL1_C | SCTLR_EL1_M)

#define TCR_ELx_TxSZ		(64 - 48)
#define TCR_EL1_T0SZ		(TCR_ELx_TxSZ << 0)
#define TCR_EL1_T1SZ		(TCR_ELx_TxSZ << 16)
#define TCR_EL1_IRGN0_WBWAC	(0x1 << 8)
#define TCR_EL1_ORGN0_WBWAC	(0x1 << 10)
#define TCR_EL1_IRGN1_WBWAC	(0x1 << 24)
#define TCR_EL1_ORGN1_WBWAC	(0x1 << 26)
#define TCR_EL1_SH0_IS		(0x3 << 12)
#define TCR_EL1_TG0_4K		(0x0 << 14)
#define TCR_EL1_TG1_4K		(0x2UL << 30)
#define TCR_EL1_IPS_256TB	(0x5UL << 32)

#define TCR_EL1_EPD0		(1 << 7)
#define TCR_EL1_EPD1		(1 << 23)
#define TCR_EL1_AS		(1UL << 36)

#define ID_AA64MMFR0_ASID_SHIFT	4
#define ID_AA64MMFR0_ASID_MASK	0xf
#define ID_AA64MMFR0_ASID_16	0x2

#define TCR_SETTINGS \
	(TCR_EL1_IPS_256TB | TCR_EL1_SH0_IS | \
	 TCR_EL1_ORGN0_WBWAC | TCR_EL1_IRGN0_WBWAC | \
	 TCR_EL1_ORGN1_WBWAC | TCR_EL1_IRGN1_WBWAC | \
	 TCR_EL1_T0SZ | TCR_EL1_T1SZ | \
	 TCR_EL1_TG0_4K | TCR_EL1_TG1_4K)

#define TTBR_ASID_SHIFT		48

#define MAIR	MAIR_EL1
#define TTBR0	TTBR0_EL1
#define TTBR1	TTBR1_EL1
#define MPIDR	MPIDR_EL1
#define	TCR	TCR_EL1

#define SPSR_EL(spsr)		(((spsr) & 0xc) >> 2)

/*
 * Values used to bring a CPU from its reset EL down to EL1 with the MMU
 * off, shared by the boot preamble and the secondary bring-up path.
 */

/* SCR_EL3: NS below EL3, HVC enabled, next lower EL is AArch64 (RES1[5:4]). */
#define SCR_EL3_NS		(1 << 0)
#define SCR_EL3_RES1		(3 << 4)
#define SCR_EL3_SMD		(1 << 7)
#define SCR_EL3_HCE		(1 << 8)
#define SCR_EL3_RW		(1 << 10)
#define SCR_EL3_INIT \
	(SCR_EL3_NS | SCR_EL3_RES1 | SCR_EL3_SMD | SCR_EL3_HCE | SCR_EL3_RW)

/* HCR_EL2: EL1 is AArch64 (RW); set/way ops trap for SW I/O coherency (SWIO). */
#define HCR_EL2_SWIO		(1 << 1)
#define HCR_EL2_RW		(1 << 31)
#define HCR_EL2_INIT		(HCR_EL2_RW | HCR_EL2_SWIO)

/* CPACR_EL1: do not trap FP/SIMD at EL0/EL1. */
#define CPACR_EL1_FPEN		(3 << 20)

/* SCTLR_EL1 reset value: architected RES1 bits set, MMU and caches off. */
#define SCTLR_EL1_RES1		0x30d00800

/* SPSR_ELx eret target: mode in M[3:0], DAIF masked in [9:6]. */
#define SPSR_MODE_EL1t		0x4
#define SPSR_MODE_EL2h		0x9
#define SPSR_DAIF_MASKED	(0xf << 6)
#define SPSR_EL2H_MASKED	(SPSR_DAIF_MASKED | SPSR_MODE_EL2h)
#define SPSR_EL1T_MASKED	(SPSR_DAIF_MASKED | SPSR_MODE_EL1t)

/* DAIFSet/DAIFClr immediate bits: bit3 D(ebug) bit2 A(SError) bit1 I(RQ) bit0 F(IQ). */
#define DAIF_FIQ		(1 << 0)
#define DAIF_IRQ		(1 << 1)

/* exception class */
#define ESR_EC_SHIFT		(26)
#define ESR_EC(esr)		GET_FIELD((esr), 31, ESR_EC_SHIFT)
#define ESR_ISS(esr)		GET_FIELD((esr), 24, 0)

#define ESR_EC_UNKNOWN		0x00
#define ESR_EC_SVC64		0x15
#define ESR_EC_IABT_LOW		0x20
#define ESR_EC_DABT_LOW		0x24

#define MPIDR_CPUID_MASK	0xff00ffffffUL

#define MPIDR_LEVEL_BITS_SHIFT	3
#define MPIDR_LEVEL_BITS	(1 << MPIDR_LEVEL_BITS_SHIFT)
#define MPIDR_LEVEL_MASK	((1 << MPIDR_LEVEL_BITS) - 1)

#define MPIDR_LEVEL_SHIFT(level) \
        (((1 << (level)) >> 1) << MPIDR_LEVEL_BITS_SHIFT)

#define MPIDR_AFFINITY_LEVEL(mpidr, level) \
        (((mpidr) >> MPIDR_LEVEL_SHIFT(level)) & MPIDR_LEVEL_MASK)

#define SYSREG_32(op1, crn, crm, op2)	s3_##op1 ##_##crn ##_##crm ##_##op2

#define ICC_IAR1_EL1		SYSREG_32(0, c12, c12, 0)
#define ICC_EOIR1_EL1		SYSREG_32(0, c12, c12, 1)
#define ICC_PMR_EL1		SYSREG_32(0, c4, c6, 0)
#define ICC_CTLR_EL1		SYSREG_32(0, c12, c12, 4)
#define ICC_IGRPEN1_EL1		SYSREG_32(0, c12, c12, 7)

#define ICC_IGRPEN1_EN		0x1

#ifndef __ASSEMBLY__

#define arm_write_sysreg(sysreg, val) \
	asm volatile ("msr	"__stringify(sysreg)", %0\n" \
			: : "r" ((u64)(val)))

#define arm_read_sysreg(sysreg, val) \
	asm volatile ("mrs	%0,  "__stringify(sysreg)"\n" : "=r" ((val)))

#endif /* __ASSEMBLY__ */
