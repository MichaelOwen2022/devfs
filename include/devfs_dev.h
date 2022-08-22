/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVFS_DEV_H__
#define __DEVFS_DEV_H__

int devfs_chdev_register(const char *name, const struct devfs_inode_ops *ops, void *data);
int devfs_chdev_unregister(const char *name);

int devfs_blkdev_register(const char *name, const struct devfs_blkdev_ops *ops, void *data);
int devfs_blkdev_unregister(const char *name);

#define _CIOCBASE           (0x0800) /* Character driver ioctl commands */
#define _BIOCBASE           (0x0900) /* Block driver ioctl commands */
#define _MTDIOCBASE         (0x0A00) /* MTD ioctl commands */

#define _IOC(type,nr)       ((type)|(nr))

/* Character driver ioctl commands */

struct led_onoff {
    unsigned char led;
    unsigned char onoff;
};

struct led_brightness {
    unsigned char led;
    unsigned char brightness;
};

#define LEDIOC_SET_ONOFF        _IOC(_CIOCBASE, 0x0080)
#define LEDIOC_SET_BRIGHTNESS   _IOC(_CIOCBASE, 0x0081)

/* Block driver ioctl commands */
#define BIOC_XIPBASE            _IOC(_BIOCBASE, 0x0001)
#define BIOC_JEDEC_ID           _IOC(_BIOCBASE, 0x0002)
#define BIOC_GEOMETRY           _IOC(_BIOCBASE, 0x0003)
#define BIOC_FLUSH              _IOC(_BIOCBASE, 0x0004)

/* MTD ioctl commands */

struct mtddev_geometry_t {
    unsigned int blocksize;     /* Size of one read/write block. */
    unsigned int erasesize;     /* Size of one erase blocks -- must be a multiple of blocksize. */
    unsigned int neraseblocks;  /* Number of erase blocks */
};

struct mtddev_erase_t {
    unsigned int seraseblock;  /* Start of erase block */
    unsigned int neraseblocks; /* Number of erase blocks */
};

#define MTDIOC_GEOMETRY         _IOC(_MTDIOCBASE, 0x0001)
#define MTDIOC_ERASE            _IOC(_MTDIOCBASE, 0x0002)

#endif/*__DEVFS_DEV_H__*/
