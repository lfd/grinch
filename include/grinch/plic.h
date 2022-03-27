#ifndef _PLIC_H
#define _PLIC_H

typedef int (*irq_handler_t)(void *userdata);

int plic_init(void);

void plic_disable_irq(unsigned long hart, u32 irq);
void plic_enable_irq(unsigned long hart, u32 irq, u32 prio, u32 thres);
int plic_register_handler(u32 irq, irq_handler_t handler, void *userdata);

int plic_handle_irq(void);

#endif /* _PLIC_H */
