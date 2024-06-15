/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/bitmap.h>
#include <grinch/device.h>
#include <grinch/list.h>
#include <grinch/types.h>
#include <grinch/iores.h>

#define PCI_VENDOR_ID		0x00	/* 16 bits */
#define PCI_DEVICE_ID		0x02	/* 16 bits */
#define PCI_COMMAND		0x04	/* 16 bits */
#define  PCI_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY	0x2	/* Enable response in Memory space */
#define  PCI_COMMAND_MASTER	0x4	/* Enable bus mastering */
#define PCI_STATUS		0x06	/* 16 bits */

#define PCI_CLASS_REVISION	0x08

#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */
#define PCI_BASE_ADDRESS_1	0x14	/* 32 bits [htype 0,1 only] */
#define PCI_BASE_ADDRESS_2	0x18	/* 32 bits [htype 0 only] */
#define PCI_BASE_ADDRESS_3	0x1c	/* 32 bits */
#define PCI_BASE_ADDRESS_4	0x20	/* 32 bits */
#define PCI_BASE_ADDRESS_5	0x24	/* 32 bits */

#define PCI_VENDOR_ID_NOTVALID	0xffff

#define PCI_NUM_BARS	6

#define PCI_BDF_PARAMS(bdf)	(bdf) >> 8, ((bdf) >> 3) & 0x1f, (bdf) & 7
#define PCI_FMT			"%02x:%02x.%x"

#define _pci_prr(__dev, header, fmt, ...)	_prr(header, "PCI: " PCI_FMT ": %s: " fmt, PCI_BDF_PARAMS((__dev)->bdf), (__dev)->dev.name, ##__VA_ARGS__)
#define _pci_prri(__dev, header, fmt, ...)	_prri(header, "PCI: " PCI_FMT ": %s: " fmt, PCI_BDF_PARAMS((__dev)->bdf), (__dev)->dev.name, ##__VA_ARGS__)

#define pci_pr(dev, fmt, ...)	_pci_prr(dev, PR_DEFAULT, fmt, ##__VA_ARGS__)
#define pci_pri(dev, fmt, ...)	_pci_prri(dev, PR_DEFAULT, fmt, ##__VA_ARGS__)

#define pci_pr_crit(dev, fmt, ...) _pci_prr(dev, PR_CRIT, fmt, ##__VA_ARGS__)
#define pci_pri_crit(dev, fmt, ...) _pci_prri(dev, PR_CRIT, fmt, ##__VA_ARGS__)

#define pci_pr_warn(dev, fmt, ...) _pci_prr(dev, PR_WARN, fmt, ##__VA_ARGS__)
#define pci_pri_warn(dev, fmt, ...) _pci_prri(dev, PR_WARN, fmt, ##__VA_ARGS__)

#define pci_pr_info(dev, fmt, ...) _pci_prr(dev, PR_INFO, fmt, ##__VA_ARGS__)
#define pci_pri_info(dev, fmt, ...) _pci_prri(dev, PR_INFO, fmt, ##__VA_ARGS__)

#define pci_pr_dbg(dev, fmt, ...) _pci_prr(dev, PR_DEBUG, fmt, ##__VA_ARGS__)
#define pci_pri_dbg(dev, fmt, ...) _pci_prri(dev, PR_DEBUG, fmt, ##__VA_ARGS__)

struct pci_range {
	iores_flags_t flags;
	struct mmio_area area;
	struct bitmap used;

	struct list_head ranges;
};

struct pci {
	void *mmcfg;
	u32 end_bus;

	struct list_head ranges;
};

enum pci_bar_type {
	PCI_BAR_32 = 0,
	PCI_BAR_1M = 1,
	PCI_BAR_64 = 2,
	PCI_BAR_RSVD,
};

struct pci_bar {
	bool prefetchable;
	enum pci_bar_type type;
	struct mmio_resource iomem;
};

struct pci_device {
	struct device dev;
	struct pci *pci;
	const struct pci_driver *driver;
	u16 bdf;
	struct pci_bar bars[PCI_NUM_BARS];

	void *data;
};

struct pci_device_id {
	u32 vendor;
	u32 device;
	u8 class;
	u8 subclass;
	u8 prog;
	u8 revision;
};

struct pci_driver {
	const char *name;
	const struct pci_device_id *id_table;
	int (*probe)(struct pci_device *dev, const struct pci_device_id *id);
	void (*remove)(struct pci_device *dev);
};

void pci_device_enable(struct pci_device *device);
int pci_map_bars(struct pci_device *device);

void pci_lspci(void);

void pci_scan(void);

#define DECLARE_PCI_DRIVER(NAME)	\
	static const struct pci_driver __NAME __used __section(".pci_drivers")
