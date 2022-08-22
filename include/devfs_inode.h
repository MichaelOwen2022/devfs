/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVFS_INODE_H__
#define __DEVFS_INODE_H__

enum devfs_type_t {
    devfs_type_dir,
    devfs_type_chdev,
    devfs_type_blkdev,
    devfs_type_maxnbr
};

struct devfs_file_t;

struct devfs_inode_ops {
    int (*open)(struct devfs_file_t *file);
    int (*read)(struct devfs_file_t *file, void *dest, size_t nbytes);
    int (*write)(struct devfs_file_t *file, const void *src, size_t nbytes);
    int (*lseek)(struct devfs_file_t *file, off_t off, int whence);
    int (*ioctl)(struct devfs_file_t *file, unsigned int cmd, unsigned long arg);
    int (*close)(struct devfs_file_t *file);
};

struct devfs_inode_t {
    struct list_head head;
    char name[DEVFS_NAME_MAX + 1];
    enum devfs_type_t type;
    int references;

    const struct devfs_inode_ops *dev_ops;
    void *dev_data;

    void *private_data;
};

struct blkdev_geometry_t {
    uint32_t sectorsize;
    uint32_t nsectors;
};

struct devfs_blkdev_ops {
    int (*open)(struct devfs_inode_t *inode);
    int (*read)(struct devfs_inode_t *inode, void *dst, uint32_t ssector, uint32_t nsectors);
    int (*write)(struct devfs_inode_t *inode, const void *src, uint32_t ssector, uint32_t nsectors);
    int (*ioctl)(struct devfs_inode_t *inode, unsigned int cmd, unsigned long arg);
    int (*geometry)(struct devfs_inode_t *inode, struct blkdev_geometry_t *geometry);
    int (*close)(struct devfs_inode_t *inode);
};

int devfs_inode_init(void);
int devfs_inode_exit(void);
int devfs_inode_lock(void);
int devfs_inode_unlock(void);
int devfs_inode_malloc(struct devfs_inode_t **inode, const char *name,
                       enum devfs_type_t type, const struct devfs_inode_ops *ops, void *data);
int devfs_inode_free(struct devfs_inode_t *inode);
int devfs_inode_search(struct devfs_inode_t **inode, const char *name);
int devfs_inode_search_with_type(struct devfs_inode_t **inode, const char *name, enum devfs_type_t type);
int devfs_inode_first(struct devfs_inode_t **inode);
int devfs_inode_next(struct devfs_inode_t **inode);

#endif/*__DEVFS_INODE_H__*/
