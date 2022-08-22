/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/zephyr.h>

#include "devfs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(devfs_blkdev_flash);

#ifndef CONFIG_FLASH_PAGE_LAYOUT
#error "Unsupported flash page layout"
#endif

#define FLASH_DEV_NAME  "flash"

static int devfs_blkdev_flash_read(struct devfs_inode_t *inode, void *dst, uint32_t ssector, uint32_t nsectors)
{
    const struct device *dev = (const struct device *)inode->private_data;
    size_t block_size = flash_get_write_block_size(dev);
    int retval = 0;

    retval = flash_read(dev, ssector * block_size, dst, nsectors * block_size);
    if (retval < 0) {
        LOG_ERR("read ssector[%d] nsectors[%d] block_size[%d] fail[%d]", ssector, nsectors, block_size, retval);
        return retval;
    }

    return (nsectors * block_size);
}

static int devfs_blkdev_flash_write(struct devfs_inode_t *inode, const void *src, uint32_t ssector, uint32_t nsectors)
{
    const struct device *dev = (const struct device *)inode->private_data;
    size_t block_size = flash_get_write_block_size(dev);
    int retval = 0;

    retval = flash_write(dev, ssector * block_size, src, nsectors * block_size);
    if (retval < 0) {
        LOG_ERR("write ssector[%d] nsectors[%d] block_size[%d] fail[%d]", ssector, nsectors, block_size, retval);
        return retval;
    }

    return (nsectors * block_size);
}

static int devfs_blkdev_flash_erase(struct devfs_inode_t *inode, uint32_t seraseblock, uint32_t neraseblocks)
{
    const struct device *dev = (const struct device *)inode->private_data;
    struct flash_pages_info pages_info;
    int retval = 0;

    flash_get_page_info_by_idx(dev, 0, &pages_info);

    retval = flash_erase(dev, seraseblock * pages_info.size, neraseblocks * pages_info.size);
    if (retval < 0) {
        LOG_ERR("erase seraseblock[%d] neraseblocks[%d] page_size[%d] fail[%d]", seraseblock, neraseblocks, pages_info.size, retval);
        return retval;
    }

    return 0;
}

static int devfs_blkdev_flash_ioctl(struct devfs_inode_t *inode, unsigned int cmd, unsigned long arg)
{
    const struct device *dev = (const struct device *)inode->private_data;
    int retval = 0;

    switch(cmd) {
#if defined(CONFIG_FLASH_JESD216_API)
    case BIOC_JEDEC_ID: {
        uint8_t *id = (uint8_t *)arg;

        retval = flash_read_jedec_id(dev, id);

        break;
    }
#endif

    case MTDIOC_GEOMETRY: {
        struct mtddev_geometry_t *geometry = (struct mtddev_geometry_t *)arg;
        size_t block_size = flash_get_write_block_size(dev);
        size_t page_count = flash_get_page_count(dev);
        struct flash_pages_info pages_info;

        if (geometry == NULL) {
            retval = -EINVAL;
            break;
        }

        flash_get_page_info_by_idx(dev, 0, &pages_info);

        geometry->blocksize = block_size;
        geometry->erasesize = pages_info.size;
        geometry->neraseblocks = page_count;

        LOG_INF("block_size[%d] page_size[%d] page_count[%d]", block_size, pages_info.size, page_count);
        LOG_INF("mtddev geometry: blocksize[%u] erasesize[%u] neraseblocks[%u]",
                geometry->blocksize, geometry->erasesize, geometry->neraseblocks);

        break;
    }

    case MTDIOC_ERASE: {
        struct mtddev_erase_t *erase = (struct mtddev_erase_t *)arg;

        if (erase == NULL) {
            retval = -EINVAL;
            break;
        }

        retval = devfs_blkdev_flash_erase(inode, erase->seraseblock, erase->neraseblocks);

        break;
    }

    default:
        retval = -ENOTTY;
        break;
    }

    return retval;
}

static int devfs_blkdev_flash_geometry(struct devfs_inode_t *inode, struct blkdev_geometry_t *geometry)
{
    const struct device *dev = (const struct device *)inode->private_data;
    size_t block_size = flash_get_write_block_size(dev);
    size_t page_count = flash_get_page_count(dev);
    struct flash_pages_info pages_info;

    flash_get_page_info_by_idx(dev, 0, &pages_info);

    geometry->sectorsize = block_size;
    geometry->nsectors   = (pages_info.size / block_size) * page_count;

    LOG_INF("block_size[%d] page_size[%d] page_count[%d]", block_size, pages_info.size, page_count);
    LOG_INF("blkdev geometry: sectorsize[%u] nsectors[%u]", geometry->sectorsize, geometry->nsectors);

    return 0;
}

static const struct devfs_blkdev_ops devfs_blkdev_flash_ops = {
    .read     = devfs_blkdev_flash_read,
    .write    = devfs_blkdev_flash_write,
    .ioctl    = devfs_blkdev_flash_ioctl,
    .geometry = devfs_blkdev_flash_geometry,
};

static const struct device *flash = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

static int devfs_blkdev_flash_init(const struct device *unused)
{
    ARG_UNUSED(unused);

    int rc = 0;

    if (flash == NULL) {
        LOG_ERR("flash device isn't ready");
        return -ENXIO;
    }
    LOG_INF("flash device %s is ready", flash->name);

    rc = devfs_blkdev_register(FLASH_DEV_NAME, &devfs_blkdev_flash_ops, (void *)flash);
    if (rc < 0) {
        LOG_ERR("devfs_blkdev_register(%s) fail[%d]", FLASH_DEV_NAME, rc);
        return rc;
    }

    LOG_INF("devfs_blkdev_register(%s) OK", FLASH_DEV_NAME);

    return 0;
}

SYS_INIT(devfs_blkdev_flash_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
