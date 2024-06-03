/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"devfs: " x

#include <asm/spinlock.h>

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/fs/devfs.h>
#include <grinch/fs/util.h>
#include <grinch/minmax.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

// FIXME: We have no reference counting of objects

#define CHARDEV_RINGBUF_SIZE	32

static LIST_HEAD(devfs_nodes);
static DEFINE_SPINLOCK(devfs_lock);

static ssize_t dev_zero_write(struct devfs_node *, void *, struct file_handle *,
			      const char *, size_t count)
{
	return count;
}

static ssize_t dev_zero_read(struct devfs_node *, void *, struct file_handle *h,
			     char *ubuf, size_t count)
{
	if (h->flags.is_kernel) {
		memset(ubuf, 0, count);
		return count;
	} else
		return umemset(current_task(), ubuf, 0, count);
}

static ssize_t
dev_null_read(struct devfs_node *, void *, struct file_handle *, char *, size_t)
{
	return 0;
}

static const struct devfs_ops dev_zero_fops = {
	.read = dev_zero_read,
	.write = dev_zero_write,
};

static const struct devfs_ops dev_null_fops = {
	.read = dev_null_read,
	.write = dev_zero_write,
};

static int
devfs_getdents(struct file_handle *handle, struct grinch_dirent *udents,
	       unsigned int size)
{
	struct grinch_dirent dent;
	struct devfs_node *node;
	loff_t i;
	int err;

	spin_lock(&devfs_lock);

	i = 0;
	err = 0;
	list_for_each_entry(node, &devfs_nodes, nodes) {
		if (i == handle->position)
			goto found;
		i++;
	}

	goto unlock_out;

found:
	i++;
	switch (node->type) {
		case DEVFS_REGULAR:
		case DEVFS_CHARDEV:
			dent.type = DT_REG;
			break;

		case DEVFS_SYMLINK:
			dent.type = DT_LNK;
			break;

		default:
			dent.type = DT_UNKNOWN;
			break;
	}

	err = copy_dirent(udents, handle->flags.is_kernel, &dent, node->name,
			  size);
	if (err)
		goto unlock_out;

	handle->position = i;
	err = 1;

unlock_out:
	spin_unlock(&devfs_lock);
	return err;
}

static ssize_t devfs_read(struct file_handle *fh, char *ubuf, size_t count)
{
	struct devfs_node *node;

	node = fh->fp->drvdata;
	if (node->type == DEVFS_SYMLINK)
		node = node->drvdata;

	if (node->type == DEVFS_CHARDEV) {
		return devfs_chardev_read(current_task(), node, fh, ubuf, count);
	}

	if (!node->fops || !node->fops->read)
		return -ENOSYS;

	return node->fops->read(node, node->drvdata, fh, ubuf, count);
}

static ssize_t
devfs_write(struct file_handle *fh, const char *ubuf, size_t count)
{
	struct devfs_node *node;

	node = fh->fp->drvdata;
	if (node->type == DEVFS_SYMLINK)
		node = node->drvdata;

	if (!node->fops || !node->fops->write)
		return -ENOSYS;

	return node->fops->write(node, node->drvdata, fh, ubuf, count);
}

static int
devfs_register_reader(struct file_handle *fh, char *ubuf, size_t count)
{
	struct devfs_node *node;
	struct wfe_read *reader;
	struct task *task;
	int err;

	node = fh->fp->drvdata;
	if (node->type == DEVFS_SYMLINK)
		node = node->drvdata;

	if (node->type != DEVFS_CHARDEV) {
		BUG();
		return -ENOSYS;
	}

	task = current_task();
	if (task->wfe.type != WFE_NONE)
		BUG();

	spin_lock(&node->lock);

	if (node->reader) {
		err = -EBUSY;
		goto unlock_out;
	}

	task->wfe.type = WFE_READ;

	reader = &task->wfe.read;
	reader->fh = fh;
	reader->ubuf = ubuf;
	reader->count = count;

	node->reader = reader;

	task_set_wfe(task);
	this_per_cpu()->schedule = true;

	err = 0;

unlock_out:
	spin_unlock(&node->lock);
	return err;
}

static struct devfs_node *devfs_find_node(const char *name)
{
	struct devfs_node *node;

	list_for_each_entry(node, &devfs_nodes, nodes)
		if (!strcmp(node->name, name))
			return node;

	return NULL;
}

static int devfs_stat(struct file *fp, struct stat *st)
{
	struct devfs_node *node;

	node = fp->drvdata;
	/* Null-Node is the root directory at the moment */
	if (!node) {
		st->st_mode = S_IFDIR;
		return 0;
	}

	st->st_size = PAGE_SIZE;
	switch (node->type) {
		case DEVFS_REGULAR:
		case DEVFS_CHARDEV:
			st->st_mode = S_IFCHR;
			break;

		case DEVFS_SYMLINK:
			st->st_mode = S_IFLNK;
			break;

		default:
			st->st_mode = S_IFREG;
			break;
	}

	return 0;
}

static int devfs_open(struct file *dir, struct file *filep, const char *path);

static const struct file_operations devfs_fops = {
	.read = devfs_read,
	.write = devfs_write,
	.stat = devfs_stat,
	.getdents = devfs_getdents,
	.register_reader = devfs_register_reader,
	.open = devfs_open,
};

int __init devfs_create_symlink(const char *dst, const char *src)
{
	struct devfs_node *node, *src_node;
	int err;

	src_node = NULL;

	spin_lock(&devfs_lock);
	list_for_each_entry(node, &devfs_nodes, nodes) {
		if (!strcmp(node->name, dst)) {
			err = -EEXIST;
			goto unlock_out;
		}
		if (!strcmp(node->name, src))
			src_node = node;
	}
	if (!src_node) {
		err = -ENOENT;
		goto unlock_out;
	}

	node = kzalloc(sizeof(*node));
	if (!node) {
		err = -ENOMEM;
		goto unlock_out;
	}

	node->type = DEVFS_SYMLINK;
	strncpy(node->name, dst, sizeof(node->name) - 1);
	node->drvdata = src_node;
	list_add(&node->nodes, &devfs_nodes);

	err = 0;

unlock_out:
	spin_unlock(&devfs_lock);
	return err;
}

void devfs_chardev_write(struct devfs_node *node, char c)
{
	struct wfe_read *wfe;
	struct task *task;
	ssize_t ret;

	if (node->type != DEVFS_CHARDEV)
		BUG();

	spin_lock(&node->lock);
	ringbuf_write(&node->rb, c);

	if (!node->reader) {
		spin_unlock(&node->lock);
		return;
	}

	/* Notify first blocking asynchronous reader */
	task = container_of(node->reader, struct task, wfe.read);
	spin_lock(&task->lock);
	wfe = node->reader;
	node->reader = NULL;
	spin_unlock(&node->lock);

	ret = devfs_chardev_read(task, node, wfe->fh, wfe->ubuf, wfe->count);

	regs_set_retval(&task->regs, ret);
	task->wfe.type = WFE_NONE;
	task->state = TASK_RUNNABLE;
	this_per_cpu()->schedule = true;

	spin_unlock(&task->lock);
}

ssize_t devfs_chardev_read(struct task *task, struct devfs_node *node,
			   struct file_handle *h, char *buf, size_t count)
{
	unsigned long copied;
	struct ringbuf *rb;
	unsigned int cnt;
	ssize_t ret;
	char *src;

	if (node->type != DEVFS_CHARDEV)
		BUG();

	if (!count)
		return 0;

	if (h->flags.is_kernel)
		BUG();

	rb = &node->rb;
	ret = 0;
	spin_lock(&node->lock);
	do {
		src = ringbuf_read(rb, &cnt);
		if (!cnt)
			break;
		cnt = min(cnt, count);
		ringbuf_consume(rb, cnt);

		copied = copy_to_user(task, buf, src, cnt);

		buf += cnt;
		count -= cnt;
		ret += copied;
		if (copied != cnt)
			break;
	} while (count);
	spin_unlock(&node->lock);

	if (!ret)
		return -EWOULDBLOCK;

	return ret;
}

void __init devfs_node_deinit(struct devfs_node *node)
{
	spin_lock(&node->lock);
	if (node->type == DEVFS_CHARDEV)
		ringbuf_free(&node->rb);

	if (node->reader)
		BUG();
}

int __init devfs_node_init(struct devfs_node *node)
{
	int err;

	spin_init(&node->lock);
	INIT_LIST_HEAD(&node->nodes);

	err = 0;
	if (node->type == DEVFS_SYMLINK)
		return -EINVAL;

	if (node->type == DEVFS_REGULAR)
		return 0;

	if (node->type == DEVFS_CHARDEV)
		err = ringbuf_init(&node->rb, CHARDEV_RINGBUF_SIZE);

	return err;
}

void __init devfs_node_unregister(struct devfs_node *node)
{
	spin_lock(&devfs_lock);
	list_del(&node->nodes);
	spin_unlock(&devfs_lock);
}

int __init devfs_node_register(struct devfs_node *node)
{
	struct devfs_node *tmp;
	int err;

	if (node->type == DEVFS_SYMLINK)
		return -EINVAL;

	spin_lock(&devfs_lock);
	list_for_each_entry(tmp, &devfs_nodes, nodes)
		if (!strcmp(node->name, tmp->name)) {
			err = -EEXIST;
			goto unlock_out;
		}

	list_add(&node->nodes, &devfs_nodes);

	err = 0;

unlock_out:
	spin_unlock(&devfs_lock);
	return err;
}

static int devfs_open(struct file *dir, struct file *filep, const char *path)
{
	struct devfs_node *node;
	int err;

	spin_lock(&devfs_lock);
	node = devfs_find_node(path);
	if (!node) {
		err = -ENOENT;
		goto unlock_out;
	}

	filep->fops = &devfs_fops;
	filep->drvdata = node;
	err = 0;

unlock_out:
	spin_unlock(&devfs_lock);
	return err;
}

static int devfs_mount(const struct file_system *fs, struct file *dir)
{
	dir->mode = S_IFDIR;
	dir->fops = &devfs_fops;
	dir->drvdata = NULL;

	return 0;
}

static const struct file_system_operations fs_ops_devfs = {
	.mount = devfs_mount,
};

/* This is the /dev "mount point" */
const struct file_system devfs = {
	.fs_ops = &fs_ops_devfs,
};

static struct devfs_node devfs_constants[] = {
	{
		.name = "zero",
		.type = DEVFS_REGULAR,
		.fops = &dev_zero_fops,
	},
	{
		.name = "null",
		.type = DEVFS_REGULAR,
		.fops = &dev_null_fops,
	},
};

int __init devfs_init(void)
{
	struct devfs_node *node;
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(devfs_constants); i++) {
		node = &devfs_constants[i];
		err = devfs_node_init(node);
		if (err)
			return err;

		err = devfs_node_register(node);
		if (err) {
			devfs_node_deinit(node);
			return err;
		}
	}

	return 0;
}
