/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _GRINCH_XPLIC_H
#define _GRINCH_XPLIC_H

#include <grinch/device.h>
#include <grinch/fdt.h>
#include <grinch/init.h>
#include <grinch/irqchip.h>

int __init xplic_probe(struct device *dev);

#endif /* _GRINCH_XPLIC_H */
