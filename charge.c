#include "framebuffer.h"
#include "gifdecode.h"
#include "device.h"
#include "input.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <hardware_legacy/power.h>
#include <sys/reboot.h>

#define CHARGING_GIF        "/system/usr/share/charge/battery.gif"
#define SLEEP_WAIT          0
#define DEFAULT_LEVEL_MAX   5

typedef struct _ChargeContext ChargeContext;

struct _ChargeContext
{
    FBSurface  *surface;
    GifImages  *images;
    int         max_bright;
    int         max_level;
};

static ChargeContext charge_ctx;

static void charge_on_timer(int signal)
{
    if (signal != SIGALRM)
        return;

    GifImages *imgs = charge_ctx.images;
    FBSurface *surf = charge_ctx.surface;

    int max = charge_ctx.max_bright;
    int bright = 0;
    int status = 0;
    static int index = 0;
    static int sleep = 0;

    if (sleep < SLEEP_WAIT)
        sleep++;

    if (index == 0)
        power_lock("charge");

    if (++index >= charge_ctx.max_level)
    {
        if (sleep >= SLEEP_WAIT)
        {
            power_unlock("charge");
            power_sleep(5);
        }

        index = 0;
    }

    status = battery_get_status();

    switch (status)
    {
        case BATTERY_STATUS_FULL:
            index = charge_ctx.max_level - 1;
            break;

        case BATTERY_STATUS_NOT_CHARGING:
#ifdef HAVE_ANDROID_OS
            sync();
            reboot(RB_POWER_OFF);
#endif
            break;

        default: break;
    }

    /* update light */
    if (max > 0)
    {
        bright = max / charge_ctx.max_level * (index + 1);
        light_set(bright);
    }

    /* update screen */
    if (surf && imgs)
    {
        memcpy(surf->buffer, imgs->buffer + (imgs->size * index), imgs->size);
    }
    
    alarm(1); 
}

int main(int argc, char *argv[])
{
    GifImages *imgs = NULL;
    FBSurface *surf = NULL;

#ifdef CHARGE_ENABLE_SCREEN
    surf = frame_buffer_get_default();
    imgs = gif_decode(CHARGING_GIF);

    if (surf->width == imgs->w || surf->height == imgs->h)
    {
        charge_ctx.images = imgs;
    }

    charge_ctx.surface = surf;
#else
    lcd_bright_set(0);
#endif

    charge_ctx.max_bright = light_get_max();
    charge_ctx.max_level = imgs ? imgs->count : DEFAULT_LEVEL_MAX;
    
    signal(SIGALRM, charge_on_timer);
    alarm(1);

    /* event loop */
    wait_onkey();

    /* power on */
    power_lock("PowerManagerService");
    power_unlock("charge");
    light_set(0);
    vibrator_set(500);

    frame_buffer_close();
    free(imgs);

    return 0;
}

