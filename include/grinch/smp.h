#ifndef _SMP_H
#define _SMP_H

int platform_init(void);
int smp_init(void);

extern u64 timebase_frequency;
extern unsigned long eval_ipi_target;

#endif /* _SMP_H */
