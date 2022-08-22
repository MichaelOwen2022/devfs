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

#define LED_NAME	"/dev/pwmleds"

int ioctl(int fd, unsigned long request, ...);

void main(void)
{
    int led = 0;
    int ret = 0;

    led = open(LED_NAME, O_RDWR);
    if (led < 0) {
        printk("open(%s) fail \r\n", LED_NAME);
        return;
    }

    while (1) {
        struct led_brightness config;

        config.led = 0;

        for (int brightness = 0; brightness <= 100; brightness++)
        {
            config.brightness = brightness;

            ret = ioctl(led, LEDIOC_SET_BRIGHTNESS, &config);

            k_sleep(K_MSEC(10));
        }

        for (int brightness = 100; brightness >= 0; brightness--)
        {
            config.brightness = brightness;

            ret = ioctl(led, LEDIOC_SET_BRIGHTNESS, &config);

            k_sleep(K_MSEC(10));
        }
    }

    close(led);
    printk("close(%s) OK \r\n", LED_NAME);
}
