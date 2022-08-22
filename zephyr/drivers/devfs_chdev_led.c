/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/zephyr.h>

#include "devfs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(devfs_chdev_led);

static int devfs_chdev_led_write(struct devfs_file_t *file, const void *src, size_t nbytes)
{
    const struct device *dev = (const struct device *)file->inode->private_data;
    struct led_onoff *config = (struct led_onoff *)src;
    int retval = 0;

    for (int i = 0; i < nbytes / sizeof(struct led_onoff); i++)
    {
        if (config->onoff) {
            led_on(dev, config->led);
        } else {
            led_off(dev, config->led);
        }

        config += 1;
        retval += sizeof(struct led_onoff);
    }

    return 0;
}

static int devfs_chdev_led_ioctl(struct devfs_file_t *file, unsigned int cmd, unsigned long arg)
{
    const struct device *dev = (const struct device *)file->inode->private_data;
    int retval = 0;

    switch(cmd) {
    case LEDIOC_SET_ONOFF: {
        struct led_onoff *config = (struct led_onoff *)arg;

        if (config->onoff) {
            led_on(dev, config->led);
        } else {
            led_off(dev, config->led);
        }

        break;
    }

    case LEDIOC_SET_BRIGHTNESS: {
        struct led_brightness *config = (struct led_brightness *)arg;

        led_set_brightness(dev, config->led, config->brightness);

        break;
    }

    default:
        retval = -ENOTTY;
        break;
    }

    return retval;
}

static const struct devfs_inode_ops devfs_chdev_led_ops = {
    .write = devfs_chdev_led_write,
    .ioctl = devfs_chdev_led_ioctl,
};

static int devfs_chdev_led_setup(const char *name)
{
    const struct device *dev = NULL;
    int rc = 0;

    dev = device_get_binding(name);
    if (dev == NULL) {
        LOG_ERR("device_get_binding(%s) fail", name);
        return -ENXIO;
    }

    rc = devfs_chdev_register(name, &devfs_chdev_led_ops, (void *)dev);
    if (rc < 0) {
        LOG_ERR("devfs_chdev_register(%s) fail[%d]", name, rc);
        return rc;
    }

    LOG_INF("devfs_chdev_register(%s) OK", name);

    return 0;
}

static int devfs_chdev_led_init(const struct device *unused)
{
    ARG_UNUSED(unused);

#if CONFIG_LED_GPIO
    devfs_chdev_led_setup("leds");
#endif

#if CONFIG_LED_PWM
    devfs_chdev_led_setup("pwmleds");
#endif

    return 0;
}

SYS_INIT(devfs_chdev_led_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
