#ifndef _DEVICE_H_
#define _DEVICE_H_

enum
{
    BATTERY_STATUS_UNKNOW = 0,
    BATTERY_STATUS_CHARGING,
    BATTERY_STATUS_DISCHARGING,
    BATTERY_STATUS_NOT_CHARGING,
    BATTERY_STATUS_FULL,
};

int battery_get_status();

int battery_get_capacity();

int light_get_max();

int lcd_bright_get();

void light_set(int bright);

void light_breathe();

void power_sleep(long time);

void power_lock(const char *reason);

void power_unlock(const char *reason);

void vibrator_set(int status);

void lcd_bright_set(int bright);

void lcd_gradient(int sw, int limit);

#endif/*_DEVICE_H_*/
