#ifndef _IRQ_H
#define _IRQ_H

#include <grinch/csr.h>
#include <grinch/types.h>
#include <grinch/smp.h>
#include "../../config.h"

void irq_init(unsigned long timerdelta);
int handle_irq(u64 irq);
int handle_ipi(void);
int handle_timer(void);

extern unsigned long hz;

#define US_PER_SEC	(1000UL * 1000UL)

static inline u64 get_time(void)
{
	return csr_read(time);
}

static inline u64 get_cycle(void)
{
	return csr_read(cycle);
}

static inline u64 cycles_to_us(u64 cycles)
{
	return cycles * US_PER_SEC / hz;
}

static inline u64 timer_to_us(u64 ticks)
{
	return ticks * US_PER_SEC / timebase_frequency;
}

static inline u64 us_to_cpu_cycles(u64 us)
{
	return (us * hz) / US_PER_SEC;
}

static inline void ext_enable(void)
{
	csr_set(sie, IE_EIE);
}

static inline void ext_disable(void)
{
	csr_clear(sie, IE_EIE);
}

static inline void ipi_enable(void)
{
	csr_set(sie, IE_SIE);
}

static inline void ipi_disable(void)
{
	csr_clear(sie, IE_SIE);
}

static inline void timer_enable(void)
{
	csr_set(sie, IE_TIE);
}

static inline void timer_disable(void)
{
	csr_clear(sie, IE_TIE);
}

static inline void irq_disable(void)
{
	csr_clear(sstatus, SR_SIE);
}

static inline void irq_enable(void)
{
	csr_set(sstatus, SR_SIE);
}

static inline bool is_irq(u64 cause)
{
	return !!(cause & CAUSE_IRQ_FLAG);
}

static inline unsigned long to_irq(unsigned long cause)
{
	return cause & ~CAUSE_IRQ_FLAG;
}

#endif /* _IRQ_H */
