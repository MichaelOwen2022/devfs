/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVFS_OS_H__
#define __DEVFS_OS_H__

#include <zephyr/sys/printk.h>
#include <zephyr/sys/mutex.h>

#include <zephyr/logging/log.h>

#define DEVFS_LOG_MODULE_REG(m)     LOG_MODULE_REGISTER(m)

#define DEVFS_ERROR(fmt, args...)   LOG_ERR(fmt, ##args)
#define DEVFS_WARN(fmt, args...)    LOG_WRN(fmt, ##args)
#define DEVFS_INFO(fmt, args...)    LOG_INF(fmt, ##args)
#define DEVFS_DEBUG(fmt, args...)   LOG_DBG(fmt, ##args)

#define DEVFS_ASSERT(x) __ASSERT(x, #x)

void* devfs_malloc(size_t size);

void  devfs_free(void *mem);

#define DEVFS_FOREVER   0xFFFFFFFF

typedef struct k_mutex devfs_mutex_t;

int devfs_mutex_init(devfs_mutex_t *mutex);

int devfs_mutex_lock(devfs_mutex_t *mutex, unsigned int timeout);

int devfs_mutex_unlock(devfs_mutex_t *mutex);

int devfs_mutex_free(devfs_mutex_t *mutex);

typedef struct k_sem devfs_sem_t;

int devfs_sem_init(devfs_sem_t *sem, unsigned int initial, unsigned int limit);

int devfs_sem_take(devfs_sem_t *sem, unsigned int timeout);

int devfs_sem_give(devfs_sem_t *sem);

int devfs_sem_free(devfs_sem_t *sem);

#endif/*__DEVFS_OS_H__*/
