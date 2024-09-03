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

#define dbg_fmt(x) "Bochs-GPU: " x

#include <grinch/alloc.h>
#include <grinch/const.h>
#include <grinch/errno.h>
#include <grinch/fs/vfs.h>
#include <grinch/pci.h>
#include <grinch/printk.h>
#include <grinch/types.h>
#include <grinch/mmio.h>

#include <grinch/string.h>
#include <grinch/fb/host.h>

#define VGA_ATTRIBUTE_INDEX	0x3C0
#define VGA_MISC_WRITE		0x3C2
#define BOCHS_DISPI_BASE	0x500

#define VBE_DISPI_INDEX_ID			0x0
#define VBE_DISPI_INDEX_XRES			0x1
#define VBE_DISPI_INDEX_YRES			0x2
#define VBE_DISPI_INDEX_BPP			0x3
#define VBE_DISPI_INDEX_ENABLE			0x4
#define VBE_DISPI_INDEX_BANK			0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH		0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT		0x7
#define VBE_DISPI_INDEX_X_OFFSET		0x8
#define VBE_DISPI_INDEX_Y_OFFSET		0x9
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K	0xa

#define VBE_DISPI_DISABLED			0x00
#define VBE_DISPI_ENABLED			0x01
#define VBE_DISPI_LFB_ENABLED			0x40

#define BOCHS_PIXMODES			  \
	(1 << GFB_PIXMODE_XRGB)   | \
	(1 << GFB_PIXMODE_RGB)    | \
	(1 << GFB_PIXMODE_R5G6B5)	| \
	(1 << GFB_PIXMODE_R5G5B5)

struct bochs_gpu {
	void *ctl;
	unsigned int mmio_size;
};

static void _vga_write8(struct bochs_gpu *gpu, u16 port, u8 value)
{
	mmio_write8(gpu->ctl + port, value);
}

static void _vga_write16(struct bochs_gpu *gpu, u16 port, u16 value)
{
	mmio_write16(gpu->ctl + port, value);
}

static void vga_write8(struct bochs_gpu *gpu, u16 port, u8 value)
{
	_vga_write8(gpu, port - VGA_ATTRIBUTE_INDEX + 0x400, value);
}

static void dispi_write(struct bochs_gpu *gpu, u16 reg, u16 value)
{
	_vga_write16(gpu, BOCHS_DISPI_BASE + (reg << 1), value);
}

static int bochs_set_mode(struct fb_host *host, struct gfb_mode *mode)
{
	struct bochs_gpu *gpu;
	u64 new_size;
	u32 bpp;

	gpu = fb_priv(host);

	switch (mode->pixmode) {
		case GFB_PIXMODE_XRGB:
			bpp = 32;
			break;

		case GFB_PIXMODE_RGB:
			bpp = 24;
			break;

		case GFB_PIXMODE_R5G6B5:
			bpp = 16;
			break;

		case GFB_PIXMODE_R5G5B5:
			bpp = 15;
			break;

		default:
			dev_pr_warn(host->dev, "Unknown pixmode\n");
			return -EINVAL;
	}

	new_size = mode->xres * mode->yres * ((bpp + 7) / 8);
	if (new_size > gpu->mmio_size)
		return -EIO;

	host->info.bpp = bpp;
	host->info.fb_size = new_size;
	host->info.mode = *mode;

	dispi_write(gpu, VBE_DISPI_INDEX_BPP, bpp);
	dispi_write(gpu, VBE_DISPI_INDEX_XRES, mode->xres);
	dispi_write(gpu, VBE_DISPI_INDEX_YRES, mode->yres);
	dispi_write(gpu, VBE_DISPI_INDEX_BANK, 0);
	dispi_write(gpu, VBE_DISPI_INDEX_VIRT_WIDTH, mode->xres);
	dispi_write(gpu, VBE_DISPI_INDEX_VIRT_HEIGHT, mode->yres);
	dispi_write(gpu, VBE_DISPI_INDEX_X_OFFSET, 0);
	dispi_write(gpu, VBE_DISPI_INDEX_Y_OFFSET, 0);

	return 0;
}

static int
bochs_gpu_probe(struct pci_device *dev, const struct pci_device_id *id)
{
	struct bochs_gpu *gpu;
	struct fb_host *host;
	struct gfb_mode mode;
	int err;

	err = pci_map_bars(dev);
	if (err)
		return err;

	pci_device_enable(dev);

	host = fb_host_alloc(sizeof(*gpu), &dev->dev);
	if (!host)
		return -ENOMEM;

	gpu = fb_priv(host);

	host->fb = dev->bars[0].iomem.base;
	gpu->mmio_size = dev->bars[0].iomem.phys.size;
	gpu->ctl = dev->bars[2].iomem.base;
	dev->data = gpu;

	host->info.pixmodes_supported = BOCHS_PIXMODES;

	mode.pixmode = GFB_PIXMODE_XRGB;
	mode.xres = 320;
	mode.yres = 240;

	vga_write8(gpu, VGA_MISC_WRITE, 0x1);
	vga_write8(gpu, VGA_ATTRIBUTE_INDEX, 0x20);
	dispi_write(gpu, VBE_DISPI_INDEX_ENABLE, 0);
	bochs_set_mode(host, &mode);
	dispi_write(gpu, VBE_DISPI_INDEX_ENABLE,
		    VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

	host->set_mode = bochs_set_mode;

	err = fb_host_add(host);
	if (err)
		goto dealloc_out;

	pci_pr(dev, "%s: probed\n", host->node.name);

	return 0;

//remove_out:
// 	fb_host_remove(host);

dealloc_out:
	fb_host_dealloc(host);

	return err;
}

static void bochs_gpu_remove(struct pci_device *dev)
{
	kfree(dev->data);
}

static const struct pci_device_id bochs_gpu_tbl[] = {
	{
		.vendor = 0x1234,
		.device = 0x1111,
	},
	{ /* sentinel */ }
};

DECLARE_PCI_DRIVER(bochs_gpu) = {
	.name = "bochs-gpu",
	.id_table = bochs_gpu_tbl,
	.probe = bochs_gpu_probe,
	.remove = bochs_gpu_remove,
};
