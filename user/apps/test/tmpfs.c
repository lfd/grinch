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

#include "vfs.h"

#define TMPFS_DIR	"/TEST/"
#define TMPFS_FILE1	TMPFS_DIR "file1"
#define TMPFS_FILE2	TMPFS_DIR "file2"

static const char tmpfs_payload[] = "Hello, world!";

static int tmpfs_read(const char *pathname)
{
	char buf[sizeof(tmpfs_payload)];
	int err, fd;
	ssize_t ss;

	fd = open(pathname, O_RDONLY);
	if (fd == -1) {
		perror("open");
		err = -errno;
		goto close_out;
	}

	ss = read(fd, buf, sizeof(buf));
	if (ss == -1) {
		perror("read");
		err = -errno;
		goto close_out;
	}

	if (ss != sizeof(buf)) {
		err = -EINVAL;
		goto close_out;
	}

	if (memcmp(buf, tmpfs_payload, sizeof(tmpfs_payload))) {
			err = -EINVAL;
			goto close_out;
	}

	err = 0;

close_out:
	close(fd);

	return err;
}

static int tmpfs_write(const char *fname)
{
	ssize_t written;
	int err, fd;

	fd = open(fname, O_RDWR | O_CREAT);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	written = write(fd, tmpfs_payload, sizeof(tmpfs_payload));
	if (written == -1) {
		perror("write");
		err = -errno;
		goto close_out;
	}

close_out:
	close(fd);
	return err;
}

static int tmpfs_rw(const char *pathname)
{
        char buf[sizeof(tmpfs_payload) * 2];
	int err, fd;
        ssize_t ss;
	
	fd = open(pathname, O_RDWR);
        if (fd == -1) {
                perror("open");
		return -errno;
        }

        ss = read(fd, buf, sizeof(tmpfs_payload));
        if (ss == -1) {
                perror("read");
                err = -errno;
                goto close_out;
        }
	if (ss != sizeof(tmpfs_payload)) {
		err = -EINVAL;
		goto close_out;
	}

	ss = write(fd, tmpfs_payload, sizeof(tmpfs_payload));
        if (ss == -1) {
                perror("write");
                err = -errno;
                goto close_out;
        }
	if (ss != sizeof(tmpfs_payload)) {
		err = -EINVAL;
		goto close_out;
	}

	close(fd);

	fd = open(pathname, O_RDONLY);
	if (fd == -1) {
		perror("reopen");
		return -errno;
	}

	ss = read(fd, buf, sizeof(buf));
        if (ss == -1) {
                perror("read");
                err = -errno;
                goto close_out;
        }
	if (ss != sizeof(buf)) {
		err = -EINVAL;
		goto close_out;
	}

	err = -EINVAL;
	if (memcmp(buf, tmpfs_payload, sizeof(tmpfs_payload)))
		goto close_out;

	if (memcmp(buf + sizeof(tmpfs_payload), tmpfs_payload, sizeof(tmpfs_payload)))
		goto close_out;

	err = 0;

close_out:
        close(fd);

        return err;
}

int test_tmpfs(void)
{
	int err;

	err = mkdir(TMPFS_DIR, 0);
	if (err) {
		perror("mkdir");
		return err;
	}

	printf("tmpfs: write test on %s\n", TMPFS_FILE1);
	err = tmpfs_write(TMPFS_FILE1);
	if (err) {
		perror("write test");
		return err;
	}

	printf("tmpfs: write test on %s\n", TMPFS_FILE2);
	err = tmpfs_write(TMPFS_FILE2);
	if (err) {
		perror("write test");
		return err;
	}

	printf("tmpfs: read test on %s\n", TMPFS_FILE1);
	err = tmpfs_read(TMPFS_FILE1);
	if (err) {
		perror("read test");
		return err;
	}

	printf("tmpfs: RW test on %s\n", TMPFS_FILE2);
	err = tmpfs_rw(TMPFS_FILE2);
	if (err) {
		perror("rw");
                return err;
	}

	return 0;
}
