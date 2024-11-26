/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2017
 *
 * Authors:
 *  Henning Schild <henning.schild@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint64_t u64;

#include "../common/include/grinch/const.h"
#include "../common/include/grinch/vmgrinch_header.h"
#include "../include/asm-generic/grinch_layout.h"

#if (__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
#error "Gcov format of gcc < 4.7 is not supported!"
#endif

#ifdef __ARM_EABI__
#define BITS_PER_LONG 32
#else
#define BITS_PER_LONG 64
#endif
/*
 * the following bits are heavily inspired by linux/kernel/gcov/gcc_4.7.c
 * with some slight modification
 */
#if BITS_PER_LONG >= 64
typedef long gcov_type;
#else
typedef long long gcov_type;
#endif

#if (__GNUC__ >= 7)
#define GCOV_COUNTERS			9
#elif (__GNUC__ > 5) || (__GNUC__ == 5 && __GNUC_MINOR__ >= 1)
#define GCOV_COUNTERS			10
#elif __GNUC__ == 4 && __GNUC_MINOR__ >= 9
#define GCOV_COUNTERS			9
#else
#define GCOV_COUNTERS			8
#endif

struct gcov_ctr_info {
	unsigned int num;
	gcov_type *values;
};

struct gcov_fn_info {
	struct gcov_info *key;
	unsigned int ident;
	unsigned int lineno_checksum;
	unsigned int cfg_checksum;
	struct gcov_ctr_info ctrs[0];
};

struct gcov_info {
	unsigned int version;
	struct gcov_info *next;
	unsigned int stamp;
	char *filename;
	void (*merge[GCOV_COUNTERS])(gcov_type *, unsigned int);
	unsigned int n_functions;
	struct gcov_fn_info **functions;
};
/*
 * end of linux/kernel/gcov/gcc_4.7.c
 */

static void *vmgrinch;
static ssize_t vmgrinch_size;
extern void __gcov_merge_add(gcov_type *counters, unsigned int n_counters);
extern void __gcov_init(struct gcov_info *);
extern void __gcov_dump(void);

static void *vmgrinch2current(void *virt)
{
	unsigned long virtaddr = (unsigned long)virt;
	void *ret;

	if (virt == NULL)
		return NULL;
	assert(virtaddr >= VMGRINCH_BASE &&
	       virtaddr < VMGRINCH_BASE + (unsigned long)vmgrinch_size);
	ret = (void *)(virtaddr - VMGRINCH_BASE + (unsigned long)vmgrinch);

	return ret;
}

/*
 * translate one gcov-"tree" from the grinch address space to the current
 * addresses
 */
static void translate_all_pointers(struct gcov_info *info)
{
	struct gcov_fn_info *fn_info;
	struct gcov_ctr_info *ctr_info;
	unsigned int i, j, active;

	info->next = vmgrinch2current(info->next);
	info->filename = vmgrinch2current(info->filename);
	active = 0;
	for (i = 0; i < GCOV_COUNTERS; i++) {
		if (info->merge[i]) {
			active++;
			info->merge[i] = &__gcov_merge_add;
		} else
			break;
	}
	info->functions = vmgrinch2current(info->functions);
	for (i = 0; i < info->n_functions; i++) {
		info->functions[i] = vmgrinch2current(info->functions[i]);
		fn_info = info->functions[i];
		if (fn_info) {
			fn_info->key = vmgrinch2current(fn_info->key);
			assert(fn_info->key == info);
			for (j = 0; j < active; j++) {
				ctr_info = fn_info->ctrs + j;
				ctr_info->values =
					vmgrinch2current(ctr_info->values);
			}
		}
	}
}

int main(int argc, char **argv)
{
	struct gcov_info *gcov_info_head, *info, *next;
	struct vmgrinch_header *header;
	char *errstr = NULL;
	ssize_t count, ret;
	struct stat sbuf;
	char *filename;
	int fd;

	if (argc != 2) {
		printf("Usage: %s [FILE]\n", argv[0]);
		goto out;
	}
	filename = argv[1];

	fd = open(filename, O_RDONLY);
	if (fd < 1) {
		errstr = filename;
		goto out;
	}

	ret = fstat(fd, &sbuf);
	if (ret) {
		errstr = filename;
		goto out;
	}
	vmgrinch_size = sbuf.st_size;
	vmgrinch = malloc(vmgrinch_size);
	if (vmgrinch == NULL) {
		errstr = "malloc";
		goto out_f;
	}

	count = 0;
	while (count < vmgrinch_size) {
		ret = read(fd, vmgrinch + count, vmgrinch_size - count);
		if (ret < 0 && errno != EINTR) {
			errstr = "read";
			goto out_m;
		}
		count += ret;
	}
	assert(count == vmgrinch_size);

	header = vmgrinch + 0x40;
	if (memcmp(header->signature, VMGRINCH_SIGNATURE,
		   sizeof(header->signature))) {
		errno = EINVAL;
		error(0, 0, "%s does not seem to be a vmgrinch dump",
		      filename);
		goto out_m;
	}

	gcov_info_head = vmgrinch2current((void *)header->gcov_info_head);
	if (!gcov_info_head) {
		errno = -EINVAL;
		error(0, 0, "%s does not seem to contain gcov information",
		      filename);
		goto out_m;

	}

	gcov_info_head = (void *)*(u64*)gcov_info_head;
	gcov_info_head = vmgrinch2current(gcov_info_head);
	if (!gcov_info_head) {
		errno = EINVAL;
		error(0, 0, "%s does not contain gcov information.", filename);
		goto out_m;
	}

	info = gcov_info_head;
	for (info = gcov_info_head; info; info = info->next)
		translate_all_pointers(info);

	for (info = gcov_info_head; info;) {
		/* remember next because __gcov_init changes it */
		next = info->next;
		__gcov_init(info);
		info = next;
	}
	__gcov_dump();

out_m:
	free(vmgrinch);
out_f:
	close(fd);
out:
	if (errno && errstr)
		error(errno, errno, "%s", errstr);
	return errno;
}
