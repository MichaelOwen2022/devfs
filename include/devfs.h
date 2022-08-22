/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVFS_H__
#define __DEVFS_H__

#include "devfs_cfg.h"

#define DEVFS_O_READ	0x01
#define DEVFS_O_WRITE	0x02
#define DEVFS_O_RDWR	(DEVFS_O_READ | DEVFS_O_WRITE)

#define DEVFS_SEEK_SET	0
#define DEVFS_SEEK_CUR	1
#define DEVFS_SEEK_END	2

struct devfs_file_t {
	int flags;
	struct devfs_inode_t *inode;

	off_t offset;
};

struct devfs_dir_t {
	struct devfs_inode_t *inode;
};

struct devfs_dirent_t {
	enum devfs_type_t type;
	size_t size;
	char d_name[DEVFS_NAME_MAX + 1];
};

int devfs_open(struct devfs_file_t *file, const char *path, int flags);
int devfs_read(struct devfs_file_t *file, void *buff, size_t size);
int devfs_write(struct devfs_file_t *file, const void *buff, size_t size);
int devfs_lseek(struct devfs_file_t *file, off_t off, int whence);
int devfs_ioctl(struct devfs_file_t *file, unsigned int cmd, unsigned long arg);
int devfs_close(struct devfs_file_t *file);

int devfs_opendir(struct devfs_dir_t *dir, const char *path);
int devfs_readdir(struct devfs_dir_t *dir, struct devfs_dirent_t *entry);
int devfs_closedir(struct devfs_dir_t *dir);

int devfs_mount(const char *path);
int devfs_umount(const char *path);

int devfs_stat(const char *path, struct devfs_dirent_t *entry);

#endif/*__SYSFS_H__*/
