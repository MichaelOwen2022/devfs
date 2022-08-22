/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "devfs.h"

#include "devfs_os.h"
DEVFS_LOG_MODULE_REG(devfs_blkdev);

#define INVALID_BLOCK   0xFFFFFFFF

struct devfs_blkdev_t {
    const struct devfs_blkdev_ops *ops;
    uint32_t sectorsize;
    uint32_t nsectors;

    uint32_t block;
    union {
        bool dirty;
        uint32_t align_with_4_bytes;
    };
    uint8_t  cache[0]; /* Align with 4 bytes */
};

static int devfs_blkdev_bch_flush_cache(struct devfs_inode_t *inode)
{
    struct devfs_blkdev_t *blkdev = inode->dev_data;
    int retval = 0;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    if ((blkdev->dirty == true) &&
        (blkdev->block != INVALID_BLOCK)) {
        if (blkdev->ops->write) {
            retval = blkdev->ops->write(inode, blkdev->cache, blkdev->block, 1);
            if (retval < 0) {
                return retval;
            }

            blkdev->dirty = false;
        } else {
            DEVFS_ERROR("blkdev write ops is NULL");
        }
    }

    return 0;
}

static int devfs_blkdev_bch_read_cache(struct devfs_inode_t *inode, size_t block)
{
    struct devfs_blkdev_t *blkdev = inode->dev_data;
    int retval = 0;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    if (blkdev->block != block) {
        retval = devfs_blkdev_bch_flush_cache(inode);
        if (retval < 0) {
            return retval;
        }

        blkdev->block = INVALID_BLOCK;

        retval = blkdev->ops->read(inode, blkdev->cache, block, 1);
        if (retval < 0) {
            return retval;
        }

        blkdev->block = block;
    }

    return 0;
}

static int devfs_blkdev_bch_read(struct devfs_inode_t *inode, uint8_t *buffer, off_t offset, size_t length)
{
    struct devfs_blkdev_t *blkdev = inode->dev_data;
    uint32_t block = 0;
    uint32_t nsectors = 0;
    uint32_t blkoff = 0;
    uint32_t rdbytes = 0;
    uint32_t nbytes = 0;
    int retval = 0;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    block = (offset / blkdev->sectorsize);
    blkoff = (offset - block * blkdev->sectorsize);

    if (block >= blkdev->nsectors) {
        /* Return end-of-file */
        return 0;
    }

    if (blkoff > 0) {
        retval = devfs_blkdev_bch_read_cache(inode, block);
        if (retval < 0) {
            return retval;
        }

        if ((blkoff + length) > blkdev->sectorsize) {
            nbytes = blkdev->sectorsize - blkoff;
        } else {
            nbytes = length;
        }

        memcpy(buffer, &blkdev->cache[blkoff], nbytes);

        block++;

        if (block >= blkdev->nsectors) {
            return nbytes;
        }

        rdbytes = nbytes;
        buffer += nbytes;
        length -= nbytes;
    }

    if (length >= blkdev->sectorsize) {
        nsectors = length / blkdev->sectorsize;

        if (nsectors > (blkdev->nsectors - block)) {
            nsectors = (blkdev->nsectors - block);
        }

        retval = blkdev->ops->read(inode, buffer, block, nsectors);
        if (retval < 0) {
            return retval;
        }

        block   += nsectors;
        nbytes   = nsectors * blkdev->sectorsize;
        rdbytes += nbytes;

        if (block >= blkdev->nsectors) {
            return rdbytes;
        }

        buffer += nbytes;
        length -= nbytes;
    }

    if (length > 0) {
        retval = devfs_blkdev_bch_read_cache(inode, block);
        if (retval < 0) {
            return retval;
        }

        memcpy(buffer, blkdev->cache, length);

        rdbytes += length;
    }

    return rdbytes;
}

static int devfs_blkdev_bch_write(struct devfs_inode_t *inode, const uint8_t *buffer, off_t offset, size_t length)
{
    struct devfs_blkdev_t *blkdev = inode->dev_data;
    uint32_t block = 0;
    uint32_t nsectors = 0;
    uint32_t blkoff = 0;
    uint32_t wrbytes = 0;
    uint32_t nbytes = 0;
    int retval = 0;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    block = (offset / blkdev->sectorsize);
    blkoff = (offset - block * blkdev->sectorsize);

    if (block >= blkdev->nsectors) {
        return -EFBIG;
    }

    if (blkoff > 0) {
        retval = devfs_blkdev_bch_read_cache(inode, block);
        if (retval < 0) {
            return retval;
        }

        if ((blkoff + length) > blkdev->sectorsize) {
            nbytes = blkdev->sectorsize - blkoff;
        } else {
            nbytes = length;
        }

        memcpy(&blkdev->cache[blkoff], buffer, nbytes);
        blkdev->dirty = true;

        block++;

        if (block >= blkdev->nsectors) {
            return nbytes;
        }

        wrbytes = nbytes;
        buffer += nbytes;
        length -= nbytes;
    }

    if (length >= blkdev->sectorsize) {
        nsectors = length / blkdev->sectorsize;

        if (nsectors > (blkdev->nsectors - block)) {
            nsectors = (blkdev->nsectors - block);
        }

        retval = devfs_blkdev_bch_flush_cache(inode);
        if (retval < 0) {
            return retval;
        }

        retval = blkdev->ops->write(inode, buffer, block, nsectors);
        if (retval < 0) {
            return retval;
        }

        block   += nsectors;
        nbytes   = nsectors * blkdev->sectorsize;
        wrbytes += nbytes;

        if (block >= blkdev->nsectors) {
            return wrbytes;
        }

        buffer += nbytes;
        length -= nbytes;
    }

    if (length > 0) {
        retval = devfs_blkdev_bch_read_cache(inode, block);
        if (retval < 0) {
            return retval;
        }

        memcpy(blkdev->cache, buffer, length);
        blkdev->dirty = true;

        wrbytes += length;
    }

    return wrbytes;
}

static int devfs_blkdev_open(struct devfs_file_t *file)
{
    struct devfs_inode_t  *inode  = NULL;
    struct devfs_blkdev_t *blkdev = NULL;
    int retval = 0;

    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    inode  = file->inode;
    blkdev = file->inode->dev_data;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    if (blkdev->ops->open) {
        retval = blkdev->ops->open(inode);
        if (retval < 0) {
            return retval;
        }
    }

    return 0;
}

static int devfs_blkdev_read(struct devfs_file_t *file, void *dest, size_t nbytes)
{
    struct devfs_inode_t *inode = NULL;
    int retval = 0;

    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    inode  = file->inode;

    retval = devfs_blkdev_bch_read(inode, dest, file->offset, nbytes);
    if (retval > 0) {
        file->offset += retval;
    }

    return retval;
}

static int devfs_blkdev_write(struct devfs_file_t *file, const void *src, size_t nbytes)
{
    struct devfs_inode_t *inode = NULL;
    int retval = 0;

    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    inode  = file->inode;

    retval = devfs_blkdev_bch_write(inode, src, file->offset, nbytes);
    if (retval > 0) {
        file->offset += retval;
    }

    return retval;
}

static int devfs_blkdev_lseek(struct devfs_file_t *file, off_t off, int whence)
{
    struct devfs_inode_t  *inode  = NULL;
    struct devfs_blkdev_t *blkdev = NULL;
    off_t newpos = 0;

    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    inode  = file->inode;
    blkdev = file->inode->dev_data;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    switch(whence) {
    case SEEK_CUR:
        newpos = file->offset + off;
        break;
    case SEEK_SET:
        newpos = off;
        break;
    case SEEK_END:
        newpos = (off_t)blkdev->sectorsize * (off_t)blkdev->nsectors + off;
        break;
    default:
        return -EINVAL;
    }

    if (newpos < 0) {
        return -EINVAL;
    }

    file->offset = newpos;
    return newpos;
}

static int devfs_blkdev_ioctl(struct devfs_file_t *file, unsigned int cmd, unsigned long arg)
{
    struct devfs_inode_t  *inode  = NULL;
    struct devfs_blkdev_t *blkdev = NULL;
    int retval = 0;

    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    inode  = file->inode;
    blkdev = file->inode->dev_data;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    switch(cmd) {
    case BIOC_GEOMETRY: {
        struct blkdev_geometry_t *geometry = (struct blkdev_geometry_t *)arg;

        if (geometry == NULL) {
            retval = -EINVAL;
            break;
        }

        retval = blkdev->ops->geometry(inode, geometry);

        break;
    }

    case BIOC_FLUSH: {
        retval = devfs_blkdev_bch_flush_cache(inode);
        break;
    }

    default:
        if (blkdev->ops->ioctl) {
            retval = blkdev->ops->ioctl(inode, cmd, arg);
        } else {
            retval = -ENOTTY;
        }
        break;
    }

    return retval;
}

static int devfs_blkdev_close(struct devfs_file_t *file)
{
    struct devfs_inode_t  *inode  = NULL;
    struct devfs_blkdev_t *blkdev = NULL;
    int retval = 0;

    DEVFS_ASSERT(file);
    DEVFS_ASSERT(file->inode);

    inode  = file->inode;
    blkdev = file->inode->dev_data;

    DEVFS_ASSERT(blkdev);
    DEVFS_ASSERT(blkdev->ops);

    retval = devfs_blkdev_bch_flush_cache(inode);

    if (blkdev->ops->close) {
        blkdev->ops->close(inode);
    }

    return 0;
}

static const struct devfs_inode_ops blkdev_inode_ops = {
    devfs_blkdev_open,
    devfs_blkdev_read,
    devfs_blkdev_write,
    devfs_blkdev_lseek,
    devfs_blkdev_ioctl,
    devfs_blkdev_close
};

int devfs_blkdev_register(const char *name, const struct devfs_blkdev_ops *ops, void *data)
{
    struct devfs_blkdev_t *blkdev = NULL;
    struct devfs_inode_t *inode = NULL;
    struct blkdev_geometry_t geometry = {0};
    int retval = 0;

    DEVFS_ASSERT(name);
    DEVFS_ASSERT(ops);
    DEVFS_ASSERT(ops->geometry);

    retval = devfs_inode_malloc(&inode, name, devfs_type_blkdev, &blkdev_inode_ops, NULL);
    if (retval < 0) {
        return retval;
    }

    devfs_inode_lock();

    inode->private_data = (void *)data;

    devfs_inode_unlock();

    retval = ops->geometry(inode, &geometry);
    if (retval < 0) {
        devfs_inode_free(inode);
        return retval;
    }

    DEVFS_ASSERT(geometry.sectorsize);
    DEVFS_ASSERT(geometry.nsectors);

    blkdev = devfs_malloc(sizeof(struct devfs_blkdev_t) + geometry.sectorsize);
    if (blkdev == NULL) {
        devfs_inode_free(inode);
        return -ENOMEM;
    }

    blkdev->ops = ops;
    blkdev->sectorsize = geometry.sectorsize;
    blkdev->nsectors = geometry.nsectors;
    blkdev->block = INVALID_BLOCK;
    blkdev->dirty = false;

    devfs_inode_lock();

    inode->dev_data = blkdev;

    devfs_inode_unlock();

    return 0;
}

int devfs_blkdev_unregister(const char *name)
{
    struct devfs_inode_t *inode = NULL;
    int retval = 0;

    retval = devfs_inode_search_with_type(&inode, name, devfs_type_blkdev);
    if (retval < 0) {
        return retval;
    }

    retval = devfs_inode_free(inode);
    if (retval < 0) {
        return retval;
    }

    return 0;
}
