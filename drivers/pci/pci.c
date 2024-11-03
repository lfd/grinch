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

#define dbg_fmt(x) "PCI: " x

#include <grinch/alloc.h>
#include <grinch/bitops.h>
#include <grinch/device.h>
#include <grinch/driver.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/mm.h>
#include <grinch/pci.h>
#include <grinch/mmio.h>
#include <grinch/panic.h>

#define PCI_MAX_DEVICE		32
#define PCI_MAX_FUNCTION	8

#define PCI_BASE_ADDRESS_MEM_MASK	(~0x0fUL)

#define for_each_pci_driver(X)						\
	for ((X) = (const struct pci_driver *)__pci_drivers_start;	\
	(X) < (const struct pci_driver *)__pci_drivers_end;		\
	(X)++)


/* currently we only support one PCI domain */
static struct device *pci_domain;

static inline void *pci_get_device_mmcfg_base(struct pci *pci, u16 bdf)
{
        return pci->mmcfg + ((unsigned long)bdf << 12);
}

static u32
pci_config_read(struct pci *pci, u16 bdf, u16 address, unsigned int size)
{
	void *mmcfg_addr;

	mmcfg_addr = pci_get_device_mmcfg_base(pci, bdf) + address;

	if (size == 1)
		return mmio_read8(mmcfg_addr);
	else if (size == 2)
		return mmio_read16(mmcfg_addr);
	else
		return mmio_read32(mmcfg_addr);
}

static u32
pci_dev_config_read(struct pci_device *dev, u16 address, unsigned int size)
{
	return pci_config_read(dev->pci, dev->bdf, address, size);
}

static void pci_config_write(struct pci *pci, u16 bdf, u16 address,
			     unsigned int size, u32 val)
{
	void *mmcfg_addr;

	mmcfg_addr = pci_get_device_mmcfg_base(pci, bdf) + address;

	if (size == 1)
		mmio_write8(mmcfg_addr, val);
	else if (size == 2)
		mmio_write16(mmcfg_addr, val);
	else
		mmio_write32(mmcfg_addr, val);
}

static void pci_dev_config_write(struct pci_device *dev, u16 address,
				 unsigned int size, u32 val)
{
	return pci_config_write(dev->pci, dev->bdf, address, size, val);
}

static u32 pci_dev_bar_read(struct pci_device *dev, unsigned char bar)
{
	return pci_dev_config_read(dev, PCI_BASE_ADDRESS_0 + 4 * bar, 4);
}

static void
pci_dev_bar_write(struct pci_device *dev, unsigned char bar, u32 val)
{
	return pci_dev_config_write(dev, PCI_BASE_ADDRESS_0 + 4 * bar, 4, val);
}

void pci_device_enable(struct pci_device *dev)
{
	u16 cmd;

	cmd = pci_config_read(dev->pci, dev->bdf, PCI_COMMAND, 2);
	cmd |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
	pci_config_write(dev->pci, dev->bdf, PCI_COMMAND, 2, cmd);
}

static void pci_device_disable(struct pci_device *dev)
{
	u16 cmd;

	cmd = pci_config_read(dev->pci, dev->bdf, PCI_COMMAND, 2);
	cmd &= ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pci_config_write(dev->pci, dev->bdf, PCI_COMMAND, 2, cmd);
}

static int
pci_get_free_area(struct pci *pci, enum pci_bar_type t, struct mmio_area *area)
{
	struct pci_range *range;
	long start;
	size_t sz;

	sz = page_up(area->size);

	list_for_each_entry(range, &pci->ranges, ranges) {
		if (range->flags & IORES_MEM_64 && t != PCI_BAR_64)
			continue;

		start = mm_bitmap_find_and_allocate(&range->used, PAGES(sz),
						    -1, sz);
		if (start == -ENOMEM)
			continue;
		else if (start < 0) {
			pr_warn("Unable to allocate in range: %pe\n",
			        ERR_PTR(start));
			return start;
		}

		area->paddr = range->area.paddr + start * PAGE_SIZE;

		return 0;
	}

	return -ENOMEM;
}

static bool pci_in_range(struct pci_range *range, struct mmio_area *area)
{
	if (area->paddr >= range->area.paddr &&
	    area->paddr + area->size <= range->area.paddr + range->area.size)
		return true;

	return false;
}

static void pci_put_area(struct pci *pci, struct mmio_area *area)
{
	struct pci_range *range;
	size_t sz;
	u64 base;

	sz = PAGES(page_up(area->size));

	list_for_each_entry(range, &pci->ranges, ranges) {
		if (!pci_in_range(range, area))
			continue;

		base = PAGES(area->paddr - range->area.paddr);
		bitmap_clear(range->used.bitmap, base, sz);

		return;
	}

	BUG();
}

static u32 pci_bar_test(struct pci_device *dev, unsigned int bar_no)
{
	u32 tmp, ret;

	tmp = pci_dev_bar_read(dev, bar_no);
	pci_dev_bar_write(dev, bar_no, 0xffffffff);
	ret = pci_dev_bar_read(dev, bar_no);
	pci_dev_bar_write(dev, bar_no, tmp);

	return ret;
}

static int pci_scan_bar(struct pci_device *dev, unsigned int bar_no)
{
	struct pci_bar *bar;
	u32 sz, val;
	u64 sz64;

	bar = &dev->bars[bar_no];

	/* Disallow scanning on active BARs */
	if (bar->iomem.base)
		return -EBUSY;

	if (bar->type == PCI_BAR_RSVD)
		BUG();

	val = pci_dev_bar_read(dev, bar_no);

	/* Only MMIO-BARs are supported */
	if ((val & (1 << 0)) != 0)
		return -EINVAL;

	bar->prefetchable = !!(val & (1 << 3));
	bar->type = (val >> 1) & 3;

	if (bar->type > 2)
		return -EINVAL;

	sz = pci_bar_test(dev, bar_no);
	if (sz == 0)
		return -ENOENT;
	sz64 = sz & PCI_BASE_ADDRESS_MEM_MASK;

	if (bar->type == PCI_BAR_64) {
		memset(&dev->bars[bar_no + 1], 0, sizeof(struct pci_bar));

		dev->bars[bar_no + 1].type = PCI_BAR_RSVD;
		sz = pci_bar_test(dev, bar_no + 1);
		sz64 |= (u64)sz << 32;
	}
	bar->iomem.phys.size = (1 << ffs(sz64));

	return 0;
}

static int pci_map_bar(struct pci_device *dev, unsigned int bar_no)
{
	struct pci_bar *bar;
	struct mmio_resource *res;
	int err;

	bar = &dev->bars[bar_no];
	res = &bar->iomem;

	err = pci_get_free_area(dev->pci, bar->type, &res->phys);
	if (err) {
		pr_crit(PCI_FMT ": Error finding space for device\n",
			PCI_BDF_PARAMS(dev->bdf));
		return err;
	}

	pci_dev_bar_write(dev, bar_no, res->phys.paddr & 0xffffffff);
	if (bar->type == PCI_BAR_64)
		pci_dev_bar_write(dev, bar_no + 1,
				  (res->phys.paddr >> 32) & 0xffffffff);

	err = ioremap_res(res);
	if (err) {
		pci_put_area(dev->pci, &res->phys);
		return err;
	}

	pci_pr_info(dev,
		"BAR%u: %sprefetchable, %u-Bit, size: 0x%lx, mapped: 0x%lx\n",
		bar_no, bar->prefetchable ? "" : "non-",
		bar->type == PCI_BAR_64 ? 64 : 32, res->phys.size,
		res->phys.paddr);

	return 0;
}

int pci_map_bars(struct pci_device *dev)
{
	unsigned int bar;
	int err;

	for (bar = 0; bar < PCI_NUM_BARS; bar++) {
		err = pci_scan_bar(dev, bar);
		if (err == -ENOENT)
			continue;
		if (err)
			return err;

		err = pci_map_bar(dev, bar);
		if (err) {
			pr_warn(PCI_FMT ": unable to map BAR%u: %pe\n",
				PCI_BDF_PARAMS(dev->bdf), bar, ERR_PTR(err));
			return err;
		}

		if (dev->bars[bar].type == PCI_BAR_64)
			bar++;
	}

	return 0;
}

static const struct pci_device_id * __init
pci_driver_matches(const struct pci_driver *driver, struct pci_device_id *id)
{
	const struct pci_device_id *target;

	target = driver->id_table;
	do {
		if (target->vendor == id->vendor &&
		    target->device == id->device)
			return target;
		target++;
	} while (target->vendor && target->device);

	return NULL;
}

static void pci_release_device(struct pci_device *dev)
{
	unsigned int bar_no;
	struct pci_bar *bar;

	pci_device_disable(dev);

	for (bar_no = 0; bar_no < PCI_NUM_BARS; bar_no++) {
		bar = &dev->bars[bar_no];
		if (!bar->iomem.phys.size)
			continue;

		iounmap_res(&bar->iomem);

		pci_put_area(dev->pci, &bar->iomem.phys);

		pci_dev_bar_write(dev, bar_no, 0);
		if (bar->type == PCI_BAR_64)
			pci_dev_bar_write(dev, bar_no + 1, 0);
	}

	dev_remove(&dev->dev);
	kfree(dev);
}

static int pci_get_id(struct pci *pci, u16 bdf, struct pci_device_id *id)
{
	u32 class_rev;

	id->vendor = pci_config_read(pci, bdf, PCI_VENDOR_ID, 2);
	if (id->vendor == PCI_VENDOR_ID_NOTVALID)
		return -ENOENT;

	id->device = pci_config_read(pci, bdf, PCI_DEVICE_ID, 2);

	class_rev = pci_config_read(pci, bdf, PCI_CLASS_REVISION, 4);
	id->class = (class_rev >> 24) & 0xff;
	id->subclass = (class_rev >> 16) & 0xff;
	id->prog = (class_rev >> 8) & 0xff;
	id->revision = (class_rev >> 0) & 0xff;

	return 0;
}

static int __init _pci_scan(struct device *dom)
{
	const struct pci_device_id *match;
	const struct pci_driver *driver;
	struct pci_device *pci_dev;
	struct pci_device_id id;
	u32 bdf, bdf_max;
	struct pci *pci;
	int err;

	dev_pri(dom, "Scanning PCI bus...\n");
	pci = dom->data;

	bdf_max = (pci->end_bus + 1) * PCI_MAX_DEVICE * PCI_MAX_FUNCTION;
	for (bdf = 0; bdf < bdf_max; bdf++) {
		err = pci_get_id(pci, bdf, &id);
		if (err)
			continue;

		dev_pri(dom, "Found device " PCI_FMT
		        ": Vendor: %04x Device: %04x\n",
		        PCI_BDF_PARAMS(bdf), id.vendor, id.device);

		for_each_pci_driver(driver) {
			match = pci_driver_matches(driver, &id);
			if (!match)
				continue;

			pci_dev = kzalloc(sizeof(*pci_dev));
			if (!pci_dev)
				return -ENOMEM;

			dev_init(&pci_dev->dev, driver->name);

			pci_dev->dev.data = pci_dev;
			pci_dev->pci = pci;
			pci_dev->bdf = bdf;
			pci_dev->driver = driver;
			pci_pri_info(pci_dev, "probing...\n");
			err = driver->probe(pci_dev, match);
			if (err) {
				pci_pri_warn(pci_dev, "error probing %s: %pe\n",
					     driver->name, ERR_PTR(err));
				pci_release_device(pci_dev);
				continue;
			}

			dev_add(&pci_dev->dev);
		}
	}

	return 0;
}

void __init pci_scan(void)
{
	int err;

	if (!pci_domain)
		return;

	err = _pci_scan(pci_domain);
	if (err)
		dev_pri_crit(pci_domain, "Error during scan: %pe\n",
			     ERR_PTR(err));
}

static int __init pci_configure_ranges(struct device *dev, struct pci *pci)
{
	int err, ac, sc, len, stride;
	struct pci_range *range;
	const int *ranges;
	unsigned int i;

	err = fdt_addr_sz(_fdt, dev->of.node, &ac, &sc);
	if (err)
		return err;

	ranges = fdt_getprop(_fdt, dev->of.node, "ranges", &len);
	if (!ranges)
		return -ENOENT;

	// I hope we do this correctly..
	stride = ac + 2 * sc;
	u32 vals[stride];
	u8 space_code;
	for (i = 0;; i++) {
		err = fdt_read_u32_array(_fdt, dev->of.node, "ranges", vals,
					 i * stride, stride);
		if (err != stride)
			break;

		space_code = (vals[0] >> 24) & 0x3;
		if (space_code == 0 || space_code == 1)
			continue;

		range = kzalloc(sizeof(*range));
		if (!range)
			return -ENOMEM;

		list_add(&range->ranges, &pci->ranges);

		range->flags = IORES_MEM;
		if (space_code == 3)
			range->flags |= IORES_MEM_64;

		if (vals[0] & 0x40000000)
			range->flags |= IORES_PREFETCH;

		range->area.paddr = ((u64)vals[1] << 32) | vals[2];
		range->area.size = ((u64)vals[3] << 32) | vals[4];

		range->used.bit_max = PAGES(range->area.size);
		range->used.bitmap = kzalloc(BITMAP_SIZE(range->used.bit_max));
		if (!range->used.bitmap)
			return -ENOMEM;
	}

	err = (i == 0) ? -ENOENT : 0;

	return err;
}

static void __init pci_destroy(struct device *dev)
{
	struct pci_range *range, *tmp;
	struct pci *pci;

	pci = dev->data;

        list_for_each_entry_safe(range, tmp, &pci->ranges, ranges) {
		if (range->used.bitmap)
			kfree(range->used.bitmap);

		list_del(&range->ranges);
		kfree(range);
	}

	kfree(pci);
	dev->data = NULL;
}

static int __init pci_probe(struct device *dev)
{
	u32 bus_range[2];
	struct pci *pci;
	int err;

	err = dev_map_iomem(dev);
	if (err)
		return err;

	pci = kzalloc(sizeof(*pci));
	if (!pci)
		return -ENOMEM;
	dev->data = pci;

	INIT_LIST_HEAD(&pci->ranges);

	pci->mmcfg = dev->mmio.base;

	err = fdt_read_u32_array(_fdt, dev->of.node, ISTR("bus-range"),
				 bus_range, 0, ARRAY_SIZE(bus_range));
	if (err != ARRAY_SIZE(bus_range))
		goto free_out;

	pci->end_bus = bus_range[1];
	if (dev->mmio.phys.size < (pci->end_bus + 1) * 256 * 4096) {
		err = -EINVAL;
		goto free_out;
	}

	err = pci_configure_ranges(dev, pci);
	if (err)
		goto free_out;

	pci_domain = dev;

	return 0;

free_out:
	pci_destroy(dev);

	return err;
}

static void _pci_lspci(struct device *dev)
{
	struct pci_device_id id;
	u32 bdf, bdf_max;
	struct pci *pci;
	int err;

	pci = dev->data;
	dev_pr(dev, "BusDevFun  VendorID  DeviceID  Class  Sub-Class\n");
	bdf_max = (pci->end_bus + 1) * PCI_MAX_DEVICE * PCI_MAX_FUNCTION;
	for (bdf = 0; bdf < bdf_max; bdf++) {
		err = pci_get_id(pci, bdf, &id);
		if (err)
			continue;

		dev_pr(dev, PCI_FMT "    0x%04x    0x%04x    0x%04x 0x%04x\n",
		       PCI_BDF_PARAMS(bdf), id.vendor, id.device,
		       id.class, id.subclass);
	}
}

void pci_lspci(void)
{
	if (!pci_domain)
		return;

	_pci_lspci(pci_domain);
}

static const struct of_device_id pci_matches[] = {
	        { .compatible = "pci-host-ecam-generic", .data = NULL, },
		{},
};

DECLARE_DRIVER(PCI_GENERIC, "pci-ecam", PRIO_1, NULL, pci_probe, pci_matches);
