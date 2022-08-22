/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVFS_CFG_H__
#define __DEVFS_CFG_H__

#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef CONFIG_DEVFS_NAME_MAX
#define CONFIG_DEVFS_NAME_MAX   32
#endif

#define DEVFS_NAME_MAX  CONFIG_DEVFS_NAME_MAX

#ifndef CONFIG_DEVFS_INODE_MAX
#define CONFIG_DEVFS_INODE_MAX  30
#endif

#define DEVFS_INODE_MAX CONFIG_DEVFS_INODE_MAX

#include "devfs_list.h"
#include "devfs_inode.h"
#include "devfs_dev.h"

#endif/*__DEVFS_CFG_H__*/
