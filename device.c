#include "device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define BUF_LEN_MAX     256

char *hw_file_read(const char *file, size_t len)
{
    char buf[BUF_LEN_MAX+1];
    ssize_t size;

    int fd = open(file, O_RDONLY);

    if (fd < 0)
    {
        perror(file);
        return NULL;
    }

    if (len == 0 || len > BUF_LEN_MAX)
    {
        len = BUF_LEN_MAX;
    }

    size = read(fd, buf, len);
    close(fd);

    if (size > 0)
    {
        if (buf[size] == '\n')
            buf[size] = '\0';
        else
            buf[size+1] = '\0';

        return strdup(buf);
    }

    return NULL;
}

int hw_file_read_int(const char *file, size_t len)
{
    char *text = hw_file_read(file, len);
    int value = 0;

    if (text)
    {
        value = atoi(text);
        free(text);
    }

    return value;
}

int hw_file_write(const char *file, const char *content)
{
    ssize_t size;

    int fd = open(file, O_WRONLY);

    if (fd < 0)
    {
        perror(file);
        return -1;
    }

    size = write(fd, content, strlen(content));
    close(fd);

    return size;
}

int hw_file_write_int(const char *file, int value)
{
    char buf[BUF_LEN_MAX];

    sprintf(buf, "%d", value);

    return hw_file_write(file, buf);
}

int open_hotplug_socket()
{
    struct sockaddr_nl snl;
    const int size = BUF_LEN_MAX;
    int ret;

    bzero(&snl, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;

    int s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s == -1) 
    {
        perror("socket");
        return -1;
    }

    setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    ret = bind(s, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
    if (ret < 0) 
    {
        perror("bind");
        close(s);
        return -1;
    }

    return s;
}

int battery_get_status()
{
    static char *status[] = {"Unknown", "Charging", "Discharging", "Not charging", "Full", NULL};

    char *text = hw_file_read(DEV_BATTERY_STATUS, 16);

    if (!text)
        return BATTERY_STATUS_UNKNOW;

    switch (*text)
    {
        case 'U':
            return BATTERY_STATUS_UNKNOW;

        case 'C':
            return BATTERY_STATUS_CHARGING;

        case 'D':
            return BATTERY_STATUS_DISCHARGING;

        case 'N':
            return BATTERY_STATUS_NOT_CHARGING;

        case 'F':
            return BATTERY_STATUS_FULL;

        default: break;
    }

    free(text);

    return BATTERY_STATUS_UNKNOW;
}

int battery_get_capacity()
{
    return hw_file_read_int(DEV_BATTERY_CAPACITY, 16);
}

int lcd_bright_get()
{
    return hw_file_read_int(DEV_LCD_BRIGHT, 16);
}

void lcd_bright_set(int bright)
{
    hw_file_write_int(DEV_LCD_BRIGHT, bright);
}

void lcd_gradient(int sw, int limit)
{
    int interval = 500000 / limit;
    int i = 0;

    if (sw)
    {
        for (i = limit / 2; i <= limit; i++)
        {
            lcd_bright_set(i);
            usleep(interval);
        }
    }
    else
    {
        for (i = limit; i >= 0; i--)
        {
            lcd_bright_set(i);
            usleep(interval);
        }
    }
}

void led_bright_set(const char *name, int value)
{
    char device[PATH_MAX];

    snprintf(device, PATH_MAX, DEV_LED_PATH"/%s/brightness", name);

    hw_file_write_int(device, value);
}

void led_blink_set(const char *name, int set)
{
    char device[PATH_MAX];

    if (set == 0)
    {
        led_bright_set(name, 0);
        return;
    }

    snprintf(device, PATH_MAX, DEV_LED_PATH"/%s/trigger", name);
    hw_file_write(device, "timer");

    snprintf(device, PATH_MAX, DEV_LED_PATH"/%s/brightness", name);
    hw_file_write_int(device, DEV_LED_FULL);

    snprintf(device, PATH_MAX, DEV_LED_PATH"/%s/delay_on", name);
    hw_file_write_int(device, 1000);

    snprintf(device, PATH_MAX, DEV_LED_PATH"/%s/delay_off", name);
    hw_file_write_int(device, 1000);

    return;
}

int light_get_max()
{
    return DEV_LED_FULL;
}

void light_set(int bright)
{
    hw_file_write_int(DEV_LIGHT_BRIGHT, bright);
}

void light_breathe(int orient)
{
    int max = light_get_max();
    int interval = 1000000 / max;
    int i = 0;

    for (i = 0; i < max; i++)
    {
        light_set(i);
        usleep(interval);
    }

    for (i = max; i > 0; i--)
    {
        light_set(i);
        usleep(interval);
    }
}

void vibrator_set(int status)
{
    hw_file_write_int(DEV_VIBRATOR, status);
}

void power_sleep(long time)
{
    hw_file_write(DEV_POWER_STATE, "mem");
}

void power_lock(const char *reason)
{
    hw_file_write(DEV_POWER_LOCK, reason);
}

void power_unlock(const char *reason)
{
    hw_file_write(DEV_POWER_UNLOCK, reason);
}

