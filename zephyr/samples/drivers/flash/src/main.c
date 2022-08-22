/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "devfs.h"

#define FLASH_NAME	"/dev/flash"

#define K(s)    (s * 1024)

int ioctl(int fd, unsigned long request, ...);

void main(void)
{
    struct mtddev_geometry_t mtddev_geometry = {0};
    struct blkdev_geometry_t blkdev_geometry = {0};
    bool need_erase = true;
    off_t skipsize = K(128); /* skip code partition */
    size_t chipsize = 0;
    int flash = 0;
    int retval = 0;

    flash = open(FLASH_NAME, O_RDWR);
    if (flash < 0) {
        printk("open(%s) fail. \r\n", FLASH_NAME);
        return;
    }
    printk("open(%s) OK. \r\n", FLASH_NAME);

    retval = ioctl(flash, MTDIOC_GEOMETRY, &mtddev_geometry);
    if (retval < 0) {
        printk("ioctl MTDIOC_GEOMETRY fail. \r\n");

        need_erase = false;

        retval = ioctl(flash, BIOC_GEOMETRY, &blkdev_geometry);
        if (retval < 0) {
            printk("ioctl BIOC_GEOMETRY fail. \r\n");

            close(flash);
            return;
        } else {
            chipsize = blkdev_geometry.sectorsize * blkdev_geometry.nsectors;
        }
    } else {
        chipsize = mtddev_geometry.erasesize * mtddev_geometry.neraseblocks;
    }

    retval = lseek(flash, skipsize, SEEK_SET);
    if (retval != skipsize) {
        printk("lseek(%d, %u, SEEK_SET) offset[%d] fail. \r\n", flash, skipsize, retval);
    } else {
        printk("lseek(%d, %u, SEEK_SET) offset[%d] OK. \r\n", flash, skipsize, retval);
    }

    for (uint32_t offset = skipsize; offset < chipsize; offset += sizeof(uint32_t)) {
        if (need_erase) {
            if ((offset % mtddev_geometry.erasesize) == 0) {
                struct mtddev_erase_t erase;

                erase.seraseblock  = offset / mtddev_geometry.erasesize;
                erase.neraseblocks = 1;

                retval = ioctl(flash, MTDIOC_ERASE, &erase);
                if (retval < 0) {
                    printk("ioctl MTDIOC_ERASE(%u, %u) fail[%d]. \r\n", erase.seraseblock, erase.neraseblocks, retval);
                } else {
                    printk("ioctl MTDIOC_ERASE(%u, %u) OK. \r\n", erase.seraseblock, erase.neraseblocks);
                }

                k_sleep(K_MSEC(100));
            }
        }

        retval = write(flash, &offset, sizeof(offset));
        if (retval != sizeof(offset)) {
            printk("write(%d, %p, %d) fail[%d]. \r\n", flash, &offset, sizeof(offset), retval);
        }
    }

    retval = lseek(flash, skipsize, SEEK_SET);
    if (retval != skipsize) {
        printk("lseek(%d, %u, SEEK_SET) offset[%d] fail. \r\n", flash, skipsize, retval);
    } else {
        printk("lseek(%d, %u, SEEK_SET) offset[%d] OK. \r\n", flash, skipsize, retval);
    }

    for (uint32_t offset = skipsize; offset < chipsize; offset += sizeof(uint32_t)) {
        uint32_t data = 0;

        if (need_erase) {
            if ((offset % mtddev_geometry.erasesize) == 0) {
                printk("read offset[0x%08x]. \r\n", offset);

                k_sleep(K_MSEC(100));
            }
        }

        retval = read(flash, &data, sizeof(data));
        if (retval != sizeof(data)) {
            printk("read(%d, %p, %d) fail[%d]. \r\n", flash, &data, sizeof(data), retval);
        }

        if (data != offset) {
            printk("compare data[%u] != offset[%u]. \r\n", data, offset);
        }
    }

    close(flash);
    printk("close(%s) OK \r\n", FLASH_NAME);
}
