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

#define LED_NAME	"/dev/leds"

int ioctl(int fd, unsigned long request, ...);

void main(void)
{
    int led = 0;
    int ret = 0;
    struct led_onoff config;

    led = open(LED_NAME, O_RDWR);
    if (led < 0) {
        printk("open(%s) fail \r\n", LED_NAME);
        return;
    }

    while (1) {
        config.led = 1;
        config.onoff = 1;
        ret = write(led, &config, sizeof(struct led_onoff));

        config.led = 0;
        config.onoff = 0;
        ret = ioctl(led, LEDIOC_SET_ONOFF, &config);

        k_sleep(K_MSEC(1000));

        config.led = 1;
        config.onoff = 0;
        ret = write(led, &config, sizeof(struct led_onoff));

        config.led = 0;
        config.onoff = 1;
        ret = ioctl(led, LEDIOC_SET_ONOFF, &config);

        k_sleep(K_MSEC(1000));
    }

    close(led);
    printk("close(%s) OK \r\n", LED_NAME);
}
