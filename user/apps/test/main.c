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
#include <sys/wait.h>

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

static int test_tty(const char *pathname)
{
	int err, fd;
	ssize_t r;

	fd = open(pathname, O_WRONLY);
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

static int test_initrd(void)
{
	int err, fd;
	char buf[64];
	ssize_t r;
	char *tmp;

	fd = open("/initrd/test.txt", O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	r = read(fd, buf, sizeof(buf));
	if (r == -1) {
		perror("read");
		return -errno;
	}

	printf("Read %lu bytes\n", r);
	printf("%s\n", buf);

	err = close(fd);
	if (err) {
		perror("close");
		return -errno;
	}

	/* And read 1 byte-wise */
	fd = open("/initrd/test.txt", O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	tmp = buf;
	memset(buf, 0, sizeof(buf));
	while (true) {
		r = read(fd, tmp, 1);
		if (r == -1) {
			perror("read");
			return -errno;
		} else if (r == 0)
			break;
		tmp++;
	}

	printf("Read %lu bytes\n", r);
	printf("%s\n", buf);

	err = close(fd);
	if (err) {
		perror("close");
		return -errno;
	}

	return 0;
}

#define NO_FORKS	50

static int test_fork(void)
{
	unsigned int i;
	pid_t child;
	int err;
	bool failed;

	failed = false;
	for (i = 0; i < NO_FORKS; i++) {
		child = fork();
		if (child == 0) {
			err = execve("/initrd/true.echse", NULL, NULL);
			perror("execve");
			failed = true;
			break;
		} else if (child == -1) {
			perror("fork!");
			failed = true;
			break;
		}
	}

	for (i = 0; ; i++) {
		child = wait(NULL);
		if (child == -1) {
			if (errno != ECHILD) {
				perror("wait");
				failed = true;
			}
			break;
		}
	}

	if (failed) {
		printf("Test failed\n");
		return -EINVAL;
	}

	err = (i == NO_FORKS) ? 0 : -EINVAL;

	return err;
}

int main(void)
{
	int err;

	printf("Testing fork+wait\n");
	err = test_fork();
	if (err)
		goto out;

	printf("Testing zero device\n");
	err = test_zero();
	if (err)
		goto out;

	printf("Testing null device\n");
	err = test_null();
	if (err)
		goto out;

	printf("Testing ttyS0\n");
	err = test_tty("/dev/ttyS0");
	if (err)
		goto out;

	printf("Testing ttySBI\n");
	err = test_tty("/dev/ttySBI");
	if (err)
		goto out;

	printf("Testing initrd\n");
	err = test_initrd();
	if (err)
		goto out;

out:
	printf("%pe\n", ERR_PTR(err));
	return err;
}
