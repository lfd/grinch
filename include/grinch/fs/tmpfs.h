/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ern Lim <ern.lim@st.oth-regensburg.de>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _FS_TMPFS_H
#define _FS_TMPFS_H

#include <grinch/fs/vfs.h>

int tmpfs_new(struct file_system *fs);

#endif /* _FS_TMPFS_H */
