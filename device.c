#include "device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define DEV_BATTERY_STATUS      "/sys/class/power_supply/battery/status"
#define DEV_BATTERY_CAPACITY    "/sys/class/power_supply/battery/capacity"
#define DEV_LIGHT_BRIGHT_MAX    "/sys/class/backlight/pxa3xx_pwm_bl2/max_brightness"
#define DEV_LIGHT_BRIGHT        "/sys/class/backlight/pxa3xx_pwm_bl2/brightness"
#define DEV_POWER_STATE         "/sys/power/state"
#define DEV_POWER_LOCK          "/sys/power/wake_lock"
#define DEV_POWER_UNLOCK        "/sys/power/wake_unlock"
#define DEV_VIBRATOR            "/sys/class/timed_output/vibrator/enable"
#define DEV_LCD_BRIGHT          "/sys/class/backlight/micco-bl/brightness"

#define BUF_LEN_MAX             256

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
    int capacity = 0;

    char *text = hw_file_read(DEV_BATTERY_CAPACITY, 16);

    if (text)
    {
        capacity = atoi(text);
        free(text);
    }

    return capacity;
}

int light_get_max()
{
    int max = 0;

    char *text = hw_file_read(DEV_LIGHT_BRIGHT_MAX, 16);

    if (text)
    {
        max = atoi(text);
        free(text);
    }

    return max;
}

void light_set(int bright)
{
    char text[16];

    snprintf(text, 16, "%d", bright);

    hw_file_write(DEV_LIGHT_BRIGHT, text);

    return;
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

void vibrator_set(int status)
{
    char buf[BUF_LEN_MAX];

    sprintf(buf, "%d", status);

    hw_file_write(DEV_VIBRATOR, buf);
}

void lcd_bright_set(int bright)
{
    char buf[BUF_LEN_MAX];

    sprintf(buf, "%d", bright);

    hw_file_write(DEV_LCD_BRIGHT, buf);
}

