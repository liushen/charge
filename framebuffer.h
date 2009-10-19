
#include <linux/fb.h>

#ifndef _FT_FRAME_BUFFER_H_
#define _FT_FRAME_BUFFER_H_

typedef struct _FBSurface FBSurface;

struct _FBSurface
{
    int     width;
    int     height;
    int     depth;
    int     size;
    char   *buffer;
};

FBSurface *frame_buffer_get_default();

int frame_buffer_get_vinfo(struct fb_var_screeninfo *vinfo);

int frame_buffer_get_finfo(struct fb_fix_screeninfo *finfo);

void frame_buffer_close();

#endif/*_FT_FRAME_BUFFER_H_*/
