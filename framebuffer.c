#include "framebuffer.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define FRAMEBUFFER_DEV_NAME    "/dev/graphics/fb0"

struct FBContext
{
    int                         fd;
    int                         screen_size;
    char                       *buffer;
    char                       *saved;
    struct fb_var_screeninfo    vinfo;
    struct fb_fix_screeninfo    finfo;
    FBSurface                   surface;
};

struct FBContext fb_context;

FBSurface *frame_buffer_get_default()
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    char *buffer;

    if (fb_context.fd)
        return &fb_context.surface;

    int fd = open(FRAMEBUFFER_DEV_NAME, O_RDWR);

    if (fd < 0) 
    {
        perror("open "FRAMEBUFFER_DEV_NAME);
        return NULL;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo)) 
    {
        perror("ioctl FBIOGET_FSCREENINFO");
        return NULL;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) 
    {
        perror("ioctl FBIOGET_VSCREENINFO");
        return NULL;
    }

    int screen_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    fb_context.saved = malloc(screen_size);
    read(fd, fb_context.saved, screen_size);

    buffer = (char *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memset(buffer, 0, screen_size);

    fb_context.fd = fd;
    fb_context.finfo = finfo;
    fb_context.vinfo = vinfo;
    fb_context.buffer = buffer;
    fb_context.screen_size = screen_size;

    fb_context.surface.width  = vinfo.xres;
    fb_context.surface.height = vinfo.yres;
    fb_context.surface.depth  = vinfo.bits_per_pixel / 8;
    fb_context.surface.buffer = buffer;
    fb_context.surface.size   = screen_size;

    printf("Mode: %dx%d %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    return &fb_context.surface;
}

int frame_buffer_get_vinfo(struct fb_var_screeninfo *vinfo)
{
    if (!fb_context.fd || vinfo == NULL)
    {
        return 0;
    }

    *vinfo = fb_context.vinfo;
    
    return 1;
}

int frame_buffer_get_finfo(struct fb_fix_screeninfo *finfo)
{
    if (!fb_context.fd || finfo == NULL)
    {
        return 0;
    }

    *finfo = fb_context.finfo;

    return 1;
}

void frame_buffer_close()
{
    if (!fb_context.fd)
    {
        return;
    }

    memcpy(fb_context.buffer, fb_context.saved, fb_context.screen_size);

    munmap(fb_context.buffer, fb_context.screen_size);
    close(fb_context.fd);
    free(fb_context.saved);

    memset(&fb_context, 0, sizeof(fb_context));
}

