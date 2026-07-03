#include <grinch/smp.h>
#include <grinch/panic.h>

void ipi_send(unsigned long cpu_id)
{
	BUG();
}

int arch_boot_cpu(unsigned long hart_id)
{
	BUG();
}

/*
 * Called unconditionally from smp_init(), so unlike arch_boot_cpu() this
 * must not trap: secondary bringup is not implemented yet.
 */
void arch_smp_bringup_init(void)
{
}
