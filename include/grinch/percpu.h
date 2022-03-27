#ifndef _PERCPU_H
#define _PERCPU_H

#include <grinch/paging.h>

#define STACK_PAGES	1
#define STACK_SIZE	(STACK_PAGES * PAGE_SIZE)
#define MAX_HARTS	64

#ifndef __ASSEMBLY__

#include <grinch/cpu.h>
#include <grinch/grinch_layout.h>
#include <grinch/symbols.h>

struct per_cpu {
	unsigned char stack[STACK_SIZE];
	union {
		unsigned char stack[STACK_SIZE];
		struct {
			unsigned char __fill[STACK_SIZE - sizeof(struct registers)];
			struct registers regs;
		};
	} exception;

	unsigned long root_table_page[PTES_PER_PT]
		__attribute__((aligned(PAGE_SIZE)));

	struct {
		u16 ctx;
	} plic;
	int hartid;
} __attribute__((aligned(PAGE_SIZE)));

static inline struct per_cpu *this_per_cpu(void)
{
	return (struct per_cpu*)PERCPU_BASE;
}

static inline unsigned long this_cpu_id(void)
{
	return this_per_cpu()->hartid;
}

static inline struct per_cpu *per_cpu(unsigned long hartid)
{
	return (struct per_cpu*)__internal_page_pool_end - (hartid + 1);
}

static inline page_table_t this_root_table_page(void)
{
	return this_per_cpu()->root_table_page;
}

#endif /* __ASSEMBLY__ */

#endif
