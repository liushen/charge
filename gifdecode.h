#include "gif_lib.h"

#ifndef _GIFDECODE_H_
#define _GIFDECODE_H__

typedef struct _GifImages GifImages;

struct _GifImages
{
    int     w, h, size, count;
    char   *buffer;
};

GifImages *gif_decode(const char *fname);

#endif/*_GIFDECODE_H__*/
