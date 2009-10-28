#include "framebuffer.h"
#include "gifdecode.h"
#include "device.h"
#include "input.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/reboot.h>

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

static void charge_on_timer(int signal)
{
    GifImages *imgs = charge_ctx.images;
    FBSurface *surf = charge_ctx.surface;
    static int index = 0;
    static int sleep = 0;
    int status = 0;

    if (index == 0)
    {
        power_lock(CHARGE_WAKE_LOCK);
#ifdef CHARGE_ENABLE_SCREEN
        lcd_gradient(1, charge_ctx.lcd_bright);
#endif
    }

    status = battery_get_status();

    if (status == BATTERY_STATUS_NOT_CHARGING)
    {
#ifdef HAVE_ANDROID_OS
        sync();
        reboot(RB_POWER_OFF);
#endif
    }

#ifdef CHARGE_ENABLE_SCREEN
    update_animation(status);
#endif

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

void *light_thread(void *arg)
{
    while (1)
    {
        light_breathe();
        usleep(500 * 1000);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    GifImages *imgs = NULL;
    FBSurface *surf = NULL;
    pthread_t pid = 0;

#ifdef CHARGE_ENABLE_SCREEN
    surf = frame_buffer_get_default();
    imgs = gif_decode(CHARGE_ANIMATION);

    if (surf->width == imgs->w || surf->height == imgs->h)
    {
        charge_ctx.images = imgs;
    }

    charge_ctx.surface = surf;
#else
    lcd_bright_set(0);
#endif

    charge_ctx.lcd_bright = lcd_bright_get();
    charge_ctx.max_level = imgs ? imgs->count - 1: CHARGE_LEVEL_MAX;
    
    pthread_create(&pid, NULL, light_thread, NULL);
    signal(SIGALRM, charge_on_timer);
    alarm(1);

    /* event loop */
    wait_onkey();

    /* power on */
    power_lock("PowerManagerService");
    power_unlock(CHARGE_WAKE_LOCK);

    lcd_bright_set(charge_ctx.lcd_bright);
    light_set(0);
    vibrator_set(500);

    frame_buffer_close();
    free(imgs);

    return 0;
}

