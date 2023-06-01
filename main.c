/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"main: " x

#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/mm.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/irq.h>
#include <grinch/serial.h>
#include <grinch/sbi.h>

/* delegate some exceptions */
#define HEDELEG				\
	(1 << EXC_LOAD_ACCESS) |	\
	(1 << EXC_LOAD_PAGE_FAULT) |	\
	(1 << EXC_STORE_ACCESS) | 	\
	(1 << EXC_STORE_PAGE_FAULT)

/* forward Soft IRQs and Timers to guest */
#define HIDELEG						\
	((1 << IRQ_S_SOFT) << VSIP_TO_HVIP_SHIFT) |	\
	((1 << IRQ_S_TIMER) << VSIP_TO_HVIP_SHIFT)

void cmain(paddr_t fdt);

static const char logo[] =
"\n\n"
"            _            _\n"
"           (_)          | |\n"
"  __ _ _ __ _ _ __   ___| |_\n"
" / _` | '__| | '_ \\ / __| '_ \\\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/\n"
"      -> Welcome to Grinch " __stringify(GRINCH_VER) " <- \n\n\n";

static unsigned long virt_pt[PTES_PER_PT * 4]
	__attribute__((aligned(4 * PAGE_SIZE)));

#define GUEST_MEM_SZ (4 * 1024 * 1024)

const void *guest_loadaddr = (const void*)0x70000000;
const void *guest_dtb = (const void*)0x60000000;

void __attribute__((noreturn)) vmreturn(struct registers *regs);

static int vmm_test(void)
{
	void *guest_code, *dtb;
	struct registers regs;
	int err;

	ps("Testing H-extensions\n");

	/*
	 * First of all, allocate some 2M pages for our guest, zero them, and
	 * copy over the guest's executable
	 */

	/* allocate & copy code */
	guest_code = page_alloc(PAGES(GUEST_MEM_SZ), MEGA_PAGE_SIZE,
				PAF_EXT);
	if (IS_ERR(guest_code))
		return PTR_ERR(guest_code);
	memcpy(guest_code, _guest_code(), _guest_code_size());

	dtb = page_zalloc(page_up(_guest_dtb_size()), PAGE_SIZE, PAF_EXT);
	if (IS_ERR(dtb))
		return PTR_ERR(dtb);
	memcpy(dtb, _guest_dtb(), _guest_dtb_size());

	/* setup G-stage translation */
	err = map_range(virt_pt, guest_loadaddr,
			virt_to_phys(guest_code), GUEST_MEM_SZ,
			PAGE_FLAGS_MEM_RWXU);
	if (err)
		goto out;

	err = map_range(virt_pt, guest_dtb,
			virt_to_phys(dtb), page_up(_guest_dtb_size()),
			PAGE_FLAGS_MEM_RWXU);
	if (err)
		goto out;

	enable_mmu_hgatp(satp_mode, virt_to_phys(virt_pt));

	/* delegate exceptions */
	csr_write(CSR_HEDELEG, HEDELEG);
	/* delegate IRQs */
	csr_write(CSR_HIDELEG, HIDELEG);

	/* disable IRQs */
	csr_write(CSR_HVIP, 0);
	csr_write(CSR_HIP, 0);
	csr_write(CSR_HIE, 0);
	csr_write(CSR_HGEIE, 0);

	/* Allow rdtime */
	csr_write(CSR_HCOUNTEREN, HCOUNTEREN_TM);

	csr_write(CSR_HTIMEDELTA, 0);
	csr_write(CSR_VSTVEC, 0);
	/* disable MMU inside guest */
	csr_write(CSR_VSATP, 0);
	csr_write(CSR_VSCAUSE, 0);
	csr_write(CSR_VSTVAL, 0);

	//csr_write(CSR_VSSTATUS, SR_SD | SR_FS_DIRTY | SR_XS_DIRTY);
	csr_write(CSR_VSSTATUS, 0);

	/* Set VM's PC */
	csr_write(sepc, guest_loadaddr);
	//csr_write(CSR_VSEPC, guest_loadaddr);
	csr_write(CSR_VSEPC, 0);

	/* Ensure that the FPU is enabled in S-Mode */
	csr_set(sstatus, SR_SPP | SR_SD | SR_FS_DIRTY | SR_XS_DIRTY);

	u64 hstatus =
		(2ULL << HSTATUS_VSXL_SHIFT) | /* Xlen 64 */
		/* No TVM */
		/* No TW */
		/* No TSR */
		/* No HU */
		HSTATUS_SPV | /* activate VMM */
		0;
	csr_write(CSR_HSTATUS, hstatus);

	/* setup guest registers */
	memset(&regs, 0, sizeof(regs));

	regs.a0 = 0;
	regs.a1 = (u64)guest_dtb;

	pr("Handing control over to virtual machine\n");
	vmreturn(&regs);

out:
	pr("Error detected.\n");
	return err;
}

void cmain(paddr_t __fdt)
{
	int err;

	_puts(logo);

	pr("Hartid: %lu\n", this_cpu_id());

	err = mm_init();
	if (err)
		goto out;

	ps("Activating final paging\n");
	err = paging_init();
	if (err)
		goto out;

	err = fdt_init(__fdt);
	if (err)
		goto out;

	err = mm_init_late();
	if (err)
		goto out;

	err = platform_init();
	if (err)
		goto out;

	err = sbi_init();
	if (err)
		goto out;

	ps("Disabling IRQs\n");
	irq_disable();

	/* Enable timers. The guest might want to have a timer. */
	timer_enable();

	/* Initialise external interrupts */
	ps("Initialising irqchip...\n");
	err = irqchip_init();
	if (err)
		goto out;

	/* enable external IRQs */
	ext_enable();

	ps("Initialising Serial...\n");
	err = serial_init();
	if (err)
		goto out;
	ps("Switched over from SBI to UART\n");

	/* Boot secondary CPUs */
	ps("Booting secondary CPUs\n");
	err = smp_init();
	if (err)
		goto out;

	/* On the NOEL, SPIE must be set before sret */
	csr_set(sstatus, SR_SPIE);
	if (1)
		err = vmm_test();

out:
	pr("End reached: %d\n", err);
}
