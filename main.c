/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"main: " x

#include <grinch/arch.h>
#include <grinch/boot.h>
#include <grinch/errno.h>
#include <grinch/percpu.h>
#include <grinch/mm.h>
#include <grinch/printk.h>
#include <grinch/string.h>

static const char logo[] =
"\n\n"
"            _            _\n"
"           (_)          | |\n"
"  __ _ _ __ _ _ __   ___| |_\n"
" / _` | '__| | '_ \\ / __| '_ \\\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/"
#ifdef ARCH_RISCV
"       on RISC-V\n"
#endif
"\n      -> Welcome to Grinch " __stringify(GRINCH_VER) " <- \n\n\n";


#undef dbg_fmt
#define dbg_fmt(x)	"memtest: " x
static void *pages[1024];
static void memtest_area(paf_t paf)
{
	unsigned int ctr, tmp;
	void *page;
	int err;
	u64 *i;

	pr("Running Memtest...\n");
	for (ctr = 0; ctr < ARRAY_SIZE(pages); ctr++) {
		page = page_alloc(1, PAGE_SIZE, paf);
		if (IS_ERR(page)) {
			pr("Err: %ld\n", PTR_ERR(page));
			break;
		}
		pr("Allocated %p -> 0x%llx\n", page, virt_to_phys(page));
		for (i = page; (void*)i < page + PAGE_SIZE; i++) {
			if (*i) {
				pr("  -> Page not zero: %p = 0x%llx\n", i, *i);
				break;
			}
		}

		pages[ctr] = page;
	}
	pr("Allocated %u pages\n", ctr);

	for (tmp = 0; tmp < ctr; tmp++) {
		pr("Freeing %p\n", pages[tmp]);
		err = page_free(pages[tmp], 1, paf);
		if (err) {
			pr("Err: %d\n", err);
			break;
		}
	}
	pr("Freed %u pages\n", ctr);
	pr("Success.\n");
}

static void memtest(void)
{
	memtest_area(PAF_INT);
	memtest_area(PAF_EXT);
}

#undef dbg_fmt
#define dbg_fmt(x)	"main: " x

void cmain(unsigned long boot_cpu, paddr_t __fdt)
{
	int err;

	puts(logo);
	err = mm_init();
	if (err)
		goto out;

	ps("Activating final paging\n");
	err = paging_init(boot_cpu);
	if (err)
		goto out;

	pr("CPU ID: %lu\n", this_cpu_id());

	err = arch_init(__fdt);
	if (err)
		goto out;

	if (0)
		memtest();

out:
	pr("End reached: %d\n", err);
}
