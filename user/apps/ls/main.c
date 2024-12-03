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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <grinch/vsprintf.h>

int main(int argc, char *argv[]);

static int ls_file_st(const char *pathname, struct stat *st)
{
	const char *type;

	switch (st->st_mode & S_IFMT) {
		case S_IFCHR:
			type = "chardev";
			break;

		case S_IFREG:
			type = "regular";
			break;

		case S_IFDIR:
			type = "directory";
			break;

		case S_IFLNK:
			type = "link";
			break;

		default:
			type = "unknown";
			break;
	}

	printf("%c%c%c%c%c%c%c%c%c %-13s %10llu	%-32s\n",
	       (st->st_mode & S_IRUSR) ? 'r' : '-',
	       (st->st_mode & S_IWUSR) ? 'w' : '-',
	       (st->st_mode & S_IXUSR) ? 'x' : '-',
	       (st->st_mode & S_IRGRP) ? 'r' : '-',
	       (st->st_mode & S_IWGRP) ? 'w' : '-',
	       (st->st_mode & S_IXGRP) ? 'x' : '-',
	       (st->st_mode & S_IROTH) ? 'r' : '-',
	       (st->st_mode & S_IWOTH) ? 'w' : '-',
	       (st->st_mode & S_IXOTH) ? 'x' : '-',
	       type, st->st_size, pathname);

	return 0;
}

static int ls_file_dirent(const char *path, struct grinch_dirent *dent)
{
	struct stat st;
	char buf[64];
	int err;

	snprintf(buf, sizeof(buf), "%s/%s", path, dent->name);

	err = stat(buf, &st);
	if (err) {
		perror("stat");
		return err;
	}

	return ls_file_st(dent->name, &st);
}

static int ls_dir(const char *path)
{
	struct grinch_dirent *dent;
	int i, fd, err;
	char buf[256];

	fd = open(path, 0);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	printf("Content of directory %s\n", path);
	for (;;) {
		err = getdents(fd, (void *)buf, sizeof(buf));
		if (err == 0)
			break;
		if (err < 0) {
			err = -errno;
			break;
		}

		dent = (void *)buf;
		for (i = 0; i < err; i++) {
			err = ls_file_dirent(path, dent);
			if (err)
				break;
			dent = (void *)(dent + 1) + strlen(dent->name);
		}
	}

	close(fd);

	return err;
}

static int ls(const char *pathname)
{
	struct stat st;
	int err;

	err = stat(pathname, &st);
	if (err == -1)
		return -errno;

	if (S_ISDIR(st.st_mode))
		return ls_dir(pathname);

	/* non-directory entry */
	return ls_file_st(pathname, &st);
}

int main(int argc, char *argv[])
{
	const char *pathname;
	char cwd[PATH_MAX];
	int err;

	if (!getcwd(cwd, sizeof(cwd)))
			return -errno;

	if (argc == 1) {
		pathname = cwd;
	} else if (argc == 2) {
		pathname = argv[1];
	} else {
		dprintf(STDERR_FILENO, "Invalid argument\n");
		return -EINVAL;
	}

	err = ls(pathname);
	if (err) {
		dprintf(STDERR_FILENO, "ls: %s\n", strerror(err));
	}

	return err;
}
