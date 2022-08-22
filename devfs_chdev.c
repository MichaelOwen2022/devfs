/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "devfs.h"

int devfs_chdev_register(const char *name, const struct devfs_inode_ops *ops, void *data)
{
    struct devfs_inode_t *inode = NULL;
    int retval = 0;

    retval = devfs_inode_malloc(&inode, name, devfs_type_chdev, ops, NULL);
    if (retval < 0) {
        return retval;
    }

    devfs_inode_lock();

    inode->private_data = data;

    devfs_inode_unlock();

    return 0;
}

int devfs_chdev_unregister(const char *name)
{
    struct devfs_inode_t *inode = NULL;
    int retval = 0;

    retval = devfs_inode_search_with_type(&inode, name, devfs_type_chdev);
    if (retval < 0) {
        return retval;
    }

    retval = devfs_inode_free(inode);
    if (retval < 0) {
        return retval;
    }

    return 0;
}
