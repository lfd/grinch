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
#include <grinch/device.h>
#include <grinch/errno.h>
#include <grinch/minmax.h>
#include <grinch/fs/devfs.h>
#include <grinch/fs/vfs.h>
#include <grinch/pci.h>
#include <grinch/printk.h>
#include <grinch/types.h>
#include <grinch/mmio.h>

#include <grinch/string.h>
#include <grinch/fb_abi.h>

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

struct bochs_gpu {
	u32 *fb;
	void *ctl;

	struct grinch_fb_screeninfo info;
	struct devfs_node node;
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

static ssize_t bochs_write(struct devfs_node *node, struct file_handle *fh,
			   const char *ubuf, size_t count)
{
	struct bochs_gpu *gpu;

	gpu = node->drvdata;
	count = min(count, gpu->info.fb_size);

	if (fh->flags.is_kernel)
		memcpy(gpu->fb, ubuf, count);
	else
		copy_from_user(current_task(), gpu->fb, ubuf, count);

	return count;
}

static int
bochs_set_mode(struct bochs_gpu *gpu, struct grinch_fb_modeinfo *mode)
{
	/* Only support 32-bit mode at the moment */
	if (mode->bpp != 32)
		return -EINVAL;

	dispi_write(gpu, VBE_DISPI_INDEX_BPP, mode->bpp);
	dispi_write(gpu, VBE_DISPI_INDEX_XRES, mode->xres);
	dispi_write(gpu, VBE_DISPI_INDEX_YRES, mode->yres);
	dispi_write(gpu, VBE_DISPI_INDEX_BANK, 0);
	dispi_write(gpu, VBE_DISPI_INDEX_VIRT_WIDTH, mode->xres);
	dispi_write(gpu, VBE_DISPI_INDEX_VIRT_HEIGHT, mode->yres);
	dispi_write(gpu, VBE_DISPI_INDEX_X_OFFSET, 0);
	dispi_write(gpu, VBE_DISPI_INDEX_Y_OFFSET, 0);

	gpu->info.fb_size = mode->xres * mode->yres * sizeof(u32);
	gpu->info.mode = *mode;

	return 0;
}

static long
bochs_ioctl(struct devfs_node *node, unsigned long op, unsigned long arg)
{
	struct grinch_fb_modeinfo mode;
	struct bochs_gpu *gpu;
	void __user *uarg;
	unsigned long ret;
	long err;

	uarg = (void __user *)arg;
	gpu = node->drvdata;

	switch (op) {
		case GRINCH_FB_SCREENINFO:
			ret = copy_to_user(current_task(), uarg, &gpu->info,
					   sizeof(gpu->info));
			if (ret != sizeof(gpu->info))
				return -EFAULT;

			err = 0;
			break;

		case GRINCH_FB_MODESET:
			ret = copy_from_user(current_task(), &mode, uarg,
					     sizeof(mode));
			if (ret != sizeof(mode))
				return -EFAULT;

			err = bochs_set_mode(gpu, &mode);
			break;

		default:
			err = -EINVAL;
	}

	return err;
}

static const struct devfs_ops bochs_fops = {
	.write = bochs_write,
	.ioctl = bochs_ioctl,
};

static int
bochs_gpu_probe(struct pci_device *dev, const struct pci_device_id *id)
{
	struct grinch_fb_modeinfo mode;
	struct bochs_gpu *gpu;
	int err;

	err = pci_map_bars(dev);
	if (err)
		return err;

	pci_device_enable(dev);

	gpu = kzalloc(sizeof(*gpu));
	if (!gpu)
		return -ENOMEM;

	gpu->fb = dev->bars[0].iomem.base;
	gpu->ctl = dev->bars[2].iomem.base;
	gpu->info.mmio_size = dev->bars[0].iomem.phys.size;
	dev->data = gpu;

	mode.bpp = 32;
	mode.xres = 320;
	mode.yres = 240;

	vga_write8(gpu, VGA_MISC_WRITE, 0x1);
	vga_write8(gpu, VGA_ATTRIBUTE_INDEX, 0x20);
	dispi_write(gpu, VBE_DISPI_INDEX_ENABLE, 0);
	bochs_set_mode(gpu, &mode);
	dispi_write(gpu, VBE_DISPI_INDEX_ENABLE,
		    VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

	/* We only have one single gpu driver, so this is okay at the moment */
	memcpy(gpu->node.name, "fb0", 4);
	gpu->node.type = DEVFS_REGULAR;
	gpu->node.fops = &bochs_fops;
	gpu->node.drvdata = gpu;
	err = devfs_node_init(&gpu->node);
	if (err)
		return err;

	err = devfs_node_register(&gpu->node);
	if (err)
		return err;

	pci_pr(dev, "%s: probed\n", gpu->node.name);

	return 0;
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
