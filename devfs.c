/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>

#include "devfs.h"

#include "devfs_os.h"
DEVFS_LOG_MODULE_REG(devfs);

static char mount_point[DEVFS_NAME_MAX] = "";

static const char* devfs_strip_mount_point(const char *path)
{
    const char *ppath = NULL;

    if (strncmp(path, mount_point, strlen(mount_point)) != 0) {
        return NULL;
    }

    ppath = path + strlen(mount_point);

    while (*ppath == '/') {
        ppath++;
    }

    return ppath;
}

int devfs_open(struct devfs_file_t *file, const char *path, int flags)
{
    int retval = 0;

    DEVFS_ASSERT(file);
    DEVFS_ASSERT(path);

    path = devfs_strip_mount_point(path);
    if (path == NULL) {
        return -ENOENT;
    }

    retval = devfs_inode_search(&file->inode, path);
    if (retval < 0) {
        return retval;
    }
    DEVFS_ASSERT(file->inode);

    file->flags = flags;

    if (file->inode->dev_ops->open) {
        retval = file->inode->dev_ops->open(file);
        if (retval < 0) {
            return retval;
        }
    }

    DEVFS_ASSERT(file->inode->references >= 0);

    file->inode->references++;

    return 0;
}

int devfs_read(struct devfs_file_t *file, void *buff, size_t size)
{
    DEVFS_ASSERT(file);
    DEVFS_ASSERT(buff);
    DEVFS_ASSERT(size);
    DEVFS_ASSERT(file->inode);

    if (!(file->flags & DEVFS_O_READ)) {
        return -EACCES;
    }

    if (!file->inode->dev_ops->read) {
        return -ENOTSUP;
    }

    return file->inode->dev_ops->read(file, buff, size);
}

int devfs_write(struct devfs_file_t *file, const void *buff, size_t size)
{
    DEVFS_ASSERT(file);
    DEVFS_ASSERT(buff);
    DEVFS_ASSERT(size);
    DEVFS_ASSERT(file->inode);

    if (!(file->flags & DEVFS_O_WRITE)) {
        return -EACCES;
    }

    if (!file->inode->dev_ops->write) {
        return -ENOTSUP;
    }

    return file->inode->dev_ops->write(file, buff, size);
}

int devfs_lseek(struct devfs_file_t *file, off_t off, int whence)
{
    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    if (!file->inode->dev_ops->lseek) {
        return -ENOTSUP;
    }

    return file->inode->dev_ops->lseek(file, off, whence);
}

int devfs_ioctl(struct devfs_file_t *file, unsigned int cmd, unsigned long arg)
{
    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    if (!file->inode->dev_ops->ioctl) {
        return -ENOTSUP;
    }

    if (cmd == ZFD_IOCTL_SET_LOCK) {
        /* do nothing */
        return 0;
    }

    return file->inode->dev_ops->ioctl(file, cmd, arg);
}

int devfs_close(struct devfs_file_t *file)
{
    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    if (file->inode == NULL) {
        return -EACCES;
    }

    if (file->inode->dev_ops->close) {
        file->inode->dev_ops->close(file);
    }

    DEVFS_ASSERT(file->inode->references > 0);

    file->inode->references--;

    file->inode = NULL;

    return 0;
}

int devfs_opendir(struct devfs_dir_t *dir, const char *path)
{
    int retval = 0;

    DEVFS_ASSERT(dir);
    DEVFS_ASSERT(path);

    path = devfs_strip_mount_point(path);
    if (path == NULL) {
        return -ENOENT;
    } else if (strlen(path) > 0) {
        return -ENOENT;
    }

    retval = devfs_inode_first((&dir->inode));
    if (retval < 0) {
        DEVFS_ERROR("devfs_inode_first fail[%d]", retval);
        return retval;
    }

    return 0;
}

int devfs_readdir(struct devfs_dir_t *dir, struct devfs_dirent_t *entry)
{
    DEVFS_ASSERT(dir);
    DEVFS_ASSERT(entry);

    if (dir->inode == NULL) {
        memset(entry, 0x00, sizeof(struct devfs_dirent_t));
    } else {
        entry->type = dir->inode->type;
        entry->size = 0;
        strncpy(entry->d_name, dir->inode->name, DEVFS_NAME_MAX);

        int retval = devfs_inode_next((&dir->inode));
        if (retval < 0) {
            DEVFS_ERROR("devfs_inode_next fail[%d]", retval);
            return retval;
        }
    }

    return 0;
}

int devfs_closedir(struct devfs_dir_t *dir)
{
    DEVFS_ASSERT(dir);

    dir->inode = NULL;
    return 0;
}

int devfs_stat(const char *path, struct devfs_dirent_t *entry)
{
    struct devfs_inode_t *inode = NULL;
    int retval = 0;

    DEVFS_ASSERT(path);
    DEVFS_ASSERT(entry);

    path = devfs_strip_mount_point(path);
    if (path == NULL) {
        return -ENOENT;
    } else if (strlen(path) == 0) {
        entry->type = devfs_type_dir;
        entry->size = 0;
        strncpy(entry->d_name, mount_point + 1, DEVFS_NAME_MAX);
        return 0;
    }

    retval = devfs_inode_search(&inode, path);
    if (retval < 0) {
        return retval;
    }

    entry->type = inode->type;
    entry->size = 0;
    strncpy(entry->d_name, inode->name, DEVFS_NAME_MAX);

    return 0;
}

int devfs_mount(const char *path)
{
    DEVFS_ASSERT(path);

    if (strlen(mount_point) > 0) {
        DEVFS_ERROR("devfs mount already");
        return -EBUSY;
    }

    memset(mount_point, 0x00, sizeof(mount_point));
    strncpy(mount_point, path, sizeof(mount_point) - 1);

    return devfs_inode_init();
}

int devfs_umount(const char *path)
{
    DEVFS_ASSERT(path);

    if (strlen(mount_point) == 0) {
        DEVFS_ERROR("devfs don't mount");
        return -EACCES;
    }

    memset(mount_point, 0x00, sizeof(mount_point));

    return devfs_inode_exit();
}
