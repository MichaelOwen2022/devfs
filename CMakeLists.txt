# Copyright (c) 2022 tangchunhui@coros.com
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_FILE_SYSTEM_DEVFS)
add_library(DEVFS INTERFACE)

target_include_directories(DEVFS INTERFACE include)

zephyr_library()
zephyr_library_sources(devfs.c devfs_os.c devfs_inode.c)
zephyr_library_sources_ifdef(CONFIG_FS_DEVFS_CHDEV devfs_chdev.c)
zephyr_library_sources_ifdef(CONFIG_FS_DEVFS_BLKDEV devfs_blkdev.c)
zephyr_library_sources_ifdef(CONFIG_FS_DEVFS_CHDEV_LED zephyr/drivers/devfs_chdev_led.c)
zephyr_library_sources_ifdef(CONFIG_FS_DEVFS_BLKDEV_FLASH zephyr/drivers/devfs_blkdev_flash.c)
zephyr_library_link_libraries(DEVFS)
target_link_libraries(DEVFS INTERFACE zephyr_interface)
endif()
