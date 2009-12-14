#undef LOG_TAG
#define LOG_TAG "Charge"

#include "framebuffer.h"
#include "gifdecode.h"
#include "device.h"
#include "input.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/reboot.h>
#include <cutils/log.h>

#define CHARGE_ANIMATION    "/system/usr/share/charge/battery.gif"
#define CHARGE_WAKE_LOCK    "charge"
#define CHARGE_WAKE_TIME    15
#define CHARGE_LEVEL_MAX    4

typedef struct _ChargeContext ChargeContext;

struct _ChargeContext
{
    FBSurface  *surface;
    GifImages  *images;
    int         lcd_bright;
    int         max_level;
};

static ChargeContext charge_ctx;

static void power_off()
{
#ifdef HAVE_ANDROID_OS
    sync();
    reboot(RB_POWER_OFF);
#endif
}

static void update_animation(int status)
{
    GifImages *imgs = charge_ctx.images;
    FBSurface *surf = charge_ctx.surface;
    int max = charge_ctx.max_level;
    static int index = 0;

    if (surf == NULL || imgs == NULL)
        return;
    
    if (status == BATTERY_STATUS_FULL)
    {
        memcpy(surf->buffer, 
                imgs->buffer + (imgs->size * max), 
                imgs->size);
        return;
    }

    memcpy(surf->buffer, imgs->buffer + (imgs->size * index), imgs->size);

    if (++index > max)
    {
        index = battery_get_capacity() * max / 100;
    }
}

static void *hotplug_thread(void *arg)
{
    int hotplug_sock = open_hotplug_socket();

    if (hotplug_sock < 0)
    {
        LOGE("open_hotplug_socket fail, ret=%d", hotplug_sock);
        return NULL;
    }

    while(1)
    {
        char buf[4096] = {0};
        recv(hotplug_sock, &buf, sizeof(buf), 0);

        if (strncmp(buf, "remove", strlen("remove")) == 0)
        {
            power_off();
        }
    }

    return NULL;
}

static void charge_on_timer(int signal)
{
    static int index = 0;
    static int sleep = 0;
    static int full = 0;

    if (index == 0)
    {
        power_lock(CHARGE_WAKE_LOCK);
#ifdef CHARGE_ENABLE_SCREEN
        lcd_gradient(1, charge_ctx.lcd_bright);
#endif
    }

    int status = battery_get_status();

    if (status == BATTERY_STATUS_NOT_CHARGING)
    {
        power_off();
    }

#ifdef CHARGE_ENABLE_SCREEN
    if (full == 0)
        update_animation(status);
#endif

    if (status == BATTERY_STATUS_FULL && full == 0)
    {
        led_blink_set("red", 0);
        led_bright_set("green", DEV_LED_FULL);
        full = 1;
    }

    if (++index >= CHARGE_WAKE_TIME)
    {
#ifdef CHARGE_ENABLE_SCREEN
        lcd_gradient(0, charge_ctx.lcd_bright);
#endif
        power_unlock(CHARGE_WAKE_LOCK);
        index = 0;

        if (sleep == 0)
        {
            power_sleep(CHARGE_WAKE_TIME);
            sleep = 1;
        }
    }

    alarm(1); 
}

int main(int argc, char *argv[])
{
    GifImages *imgs = NULL;
    FBSurface *surf = NULL;
    pthread_t tid = 0;

    charge_ctx.max_level = CHARGE_LEVEL_MAX;
    charge_ctx.lcd_bright = lcd_bright_get();

#ifdef CHARGE_ENABLE_SCREEN
    surf = frame_buffer_get_default();
    imgs = gif_decode(CHARGE_ANIMATION);

    if (surf->width == imgs->w || surf->height == imgs->h)
    {
        charge_ctx.images = imgs;
        charge_ctx.max_level = imgs->count - 1;
    }

    charge_ctx.surface = surf;
#else
    lcd_bright_set(0);
#endif

    led_blink_set("red", 1);
    pthread_create(&tid, NULL, hotplug_thread, NULL);
    signal(SIGALRM, charge_on_timer);
    alarm(1);

    // event loop
    wait_onkey();

    // power on device
    power_lock("PowerManagerService");
    power_unlock(CHARGE_WAKE_LOCK);

    lcd_bright_set(charge_ctx.lcd_bright);
    led_bright_set("red", 0);
    led_bright_set("green", 0);
    vibrator_set(500);

    frame_buffer_close();
    free(imgs);

    return 0;
}

