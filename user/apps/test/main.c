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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_SZ	64

int main(void);

APP_NAME(test);

static const char hello[] = "Hello, world!";

static int test_zero(void)
{
	int err, fd;
	char buf1[TEST_SZ], buf2[TEST_SZ];
	ssize_t r;

	memset(buf1, 0xff, sizeof(buf1));
	memset(buf2, 0x00, sizeof(buf2));

	fd = open("/dev/zero", O_RDWR);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	r = read(fd, buf1, sizeof(buf1));
	if (r == -1) {
		perror("read");
		return -errno;
	}

	if (r != TEST_SZ) {
		printf("size invalid\n");
		return -EINVAL;
	}

	if (memcmp(buf1, buf2, sizeof(buf1))) {
		printf("Unexpected read\n");
		return -EINVAL;
	}

	r = write(fd, buf1, sizeof(buf1));
	if (r == -1) {
		perror("write");
		return -errno;
	}

	if (r != TEST_SZ) {
		printf("Unable to write!\n");
		return -EINVAL;
	}

	err = close(fd);
	if (err)
		perror("close");

	return err;
}

static int test_null(void)
{
	int err, fd;
	ssize_t r;
	char buf1[TEST_SZ];

	fd = open("/dev/null", O_RDWR);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	r = read(fd, buf1, sizeof(buf1));
	if (r == -1) {
		perror("read");
		return -errno;
	}

	if (r != 0) {
		printf("size invalidn");
		return -EINVAL;
	}

	r = write(fd, buf1, sizeof(buf1));
	if (r == -1) {
		perror("write");
		return -errno;
	}

	if (r != TEST_SZ) {
		printf("invalid write: %lu!\n", r);
		return -EINVAL;
	}

	err = close(fd);
	if (err)
		perror("close");

	return err;
}

static int test_ttyS(void)
{
	int err, fd;
	ssize_t r;

	fd = open("/dev/ttyS0", O_WRONLY);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	r = write(fd, hello, sizeof(hello));
	if (r != sizeof(hello)) {
		if (r == -1) {
			perror("write");
			return -errno;
		} else {
			printf("invalid write: %ld!\n", r);
			return -EINVAL;
		}
	}

	err = close(fd);
	if (err) {
		perror("close");
		return -errno;
	}

	return 0;
}


int main(void)
{
	int err;

	printf("Testing zero device\n");
	err = test_zero();
	if (err)
		goto out;

	printf("Testing null device\n");
	err = test_null();
	if (err)
		goto out;

	printf("Testing ttyS0\n");
	err = test_ttyS();
	if (err)
		goto out;

out:
	return err;
}
