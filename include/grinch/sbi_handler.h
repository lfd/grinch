#ifndef _SBI_HANDLER_H
#define _SBI_HANDLER_H

#include <grinch/cpu.h>

int handle_ecall(struct registers *regs);

#endif /* _SBI_HANDLER_H */
