#ifndef _DEVICE_H_
#define _DEVICE_H_

#define DEV_BATTERY_STATUS      "/sys/class/power_supply/battery/status"
#define DEV_BATTERY_CAPACITY    "/sys/class/power_supply/battery/capacity"
#define DEV_LIGHT_BRIGHT        "/sys/class/leds/jogball-backlight/brightness"
#define DEV_POWER_STATE         "/sys/power/state"
#define DEV_POWER_LOCK          "/sys/power/wake_lock"
#define DEV_POWER_UNLOCK        "/sys/power/wake_unlock"
#define DEV_VIBRATOR            "/sys/class/timed_output/vibrator/enable"
#define DEV_LCD_BRIGHT          "/sys/class/backlight/micco-bl/brightness"

#define DEV_LED_PATH            "/sys/class/leds/"
#define DEV_LED_FULL            255

enum
{
    BATTERY_STATUS_UNKNOW = 0,
    BATTERY_STATUS_CHARGING,
    BATTERY_STATUS_DISCHARGING,
    BATTERY_STATUS_NOT_CHARGING,
    BATTERY_STATUS_FULL,
};

int open_hotplug_socket();

int battery_get_status();

int battery_get_capacity();

int lcd_bright_get();

void lcd_bright_set(int bright);

void lcd_gradient(int sw, int limit);

void led_bright_set(const char *name, int value);

void led_blink_set(const char *name, int set);

int light_get_max();

void light_set(int bright);

void light_breathe();

void vibrator_set(int status);

void power_sleep(long time);

void power_lock(const char *reason);

void power_unlock(const char *reason);

#endif/*_DEVICE_H_*/
