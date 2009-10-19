#include "gifdecode.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define FB_COLOR_DEPTH  16
#define FB_MAKE_COLOR_16(r, g, b) \
    (((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3))

#define GIF_COLOR_TABLE_MAX 256
#define GIF_CHECK_RETURN(cond) \
    if (!(cond)) \
    { \
        PrintGifError(); \
        return NULL; \
    }

struct GColor
{
    char r, g, b;
};

struct GColor g_color_tab[GIF_COLOR_TABLE_MAX];

static ColorMapObject* gif_find_colormap(const GifFileType* gif)
{
    ColorMapObject* cmap = gif->Image.ColorMap;

    if (cmap == NULL)
        cmap = gif->SColorMap;

    if ((unsigned)cmap->ColorCount > 256 ||
        cmap->ColorCount != (1 << cmap->BitsPerPixel))
    {
        cmap = NULL;
    }

    return cmap;
}

static int gif_find_transparent(const SavedImage *image, int colorCount)
{
    int i, index = -1;

    for (i = 0; i < image->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock* eb = image->ExtensionBlocks + i;

        if (eb->Function == 0xF9 && eb->ByteCount == 4)
        {
            if (eb->Bytes[0] & 1)
            {
                index = (unsigned char)eb->Bytes[3];

                /* check for valid index */
                if (index >= colorCount)
                    index = -1;

                break;
            }
        }
    }

    return index;
}

static void gif_init_colortable(ColorMapObject *cmap)
{
    int i = 0;

    for (; i < cmap->ColorCount; i++)
    {
        g_color_tab[i].r = cmap->Colors[i].Red;
        g_color_tab[i].g = cmap->Colors[i].Green;
        g_color_tab[i].b = cmap->Colors[i].Blue;
    }
}

static bool gif_add_image(GifImages *imgs, GifFileType *gif, int transp, uint8_t *pixels)
{
    GifImageDesc *desc = &gif->Image;
    int i = 0, k = 0;

    imgs->count  = gif->ImageCount;
    imgs->buffer = realloc(imgs->buffer, imgs->size * imgs->count);

    char *p = imgs->buffer + imgs->size * (imgs->count - 1);

    if (imgs->count > 1)
        memcpy(p, p - imgs->size, imgs->size);

    for (i = 0; i < desc->Height; i++)
    {
        for (k = 0; k < desc->Width; k++)
        {
            int index = pixels[i * desc->Width + k];

            if (transp != -1 && transp == index)
                continue;

            int offset = ((desc->Top + i) * imgs->w + desc->Left + k) * 2;

            int c16 = FB_MAKE_COLOR_16(g_color_tab[index].r, 
                                       g_color_tab[index].g,
                                       g_color_tab[index].b);
            p[offset] = c16 & 0xFF;
            p[offset + 1] = c16 >> 8;
        }
    }

    return true;
}

static int gif_read_callback(GifFileType* fileType, GifByteType* out, int size)
{
    FILE *fp = fileType->UserData;

    return fread(out, 1, size, fp);
}

GifImages *gif_decode(const char *fname)
{
    SavedImage temp_save;
    temp_save.ExtensionBlocks = NULL;
    temp_save.ExtensionBlockCount = 0;

    GifRecordType type = 0;
    GifByteType *extra = NULL;
    GifFileType *gif = NULL;
    GifImages *imgs = NULL;
    int width, height, i;
    
    FILE *fp = fopen(fname, "r");

    if (!fp)
    {
        perror(fname);
        return NULL;
    }

    gif = DGifOpen(fp, gif_read_callback);

    GIF_CHECK_RETURN(gif);

    do {
        GIF_CHECK_RETURN(DGifGetRecordType(gif, &type) != GIF_ERROR);
        
        switch (type) {
            case IMAGE_DESC_RECORD_TYPE:
            {
                GIF_CHECK_RETURN(DGifGetImageDesc(gif) != GIF_ERROR);
                GIF_CHECK_RETURN(gif->ImageCount >= 1);

                width  = gif->SWidth;
                height = gif->SHeight;

                GIF_CHECK_RETURN(width > 0 && height > 0);

                SavedImage *image = &gif->SavedImages[gif->ImageCount-1];
                GifImageDesc *desc = &image->ImageDesc;
                ColorMapObject *cmap = gif_find_colormap(gif);
            
                printf("Index: %d, top=%3d, left=%3d, width=%3d, height=%3d\n", 
                        gif->ImageCount, desc->Top, desc->Left, desc->Width, desc->Height);
            
                /* decode the colortable */
                GIF_CHECK_RETURN(cmap);
                gif_init_colortable(cmap);

                /* decode the scanlines */
                const int transp = gif_find_transparent(&temp_save, cmap->ColorCount);
                uint8_t *scanline = malloc(desc->Width * desc->Height);
                uint8_t *p = scanline;

                GIF_CHECK_RETURN(desc->Width > 0 && desc->Height > 0);

                if (gif->Image.Interlace)
                {
                    /* TODO: support interlace */
                    printf("do not support interlace now!\n");
                    return NULL;
                }

                for (i = 0; i < desc->Height; i++)
                {
                    GIF_CHECK_RETURN(DGifGetLine(gif, scanline, desc->Width) != GIF_ERROR);

                    scanline += desc->Width;
                }

                if (imgs == NULL)
                {
                    imgs = calloc(1, sizeof(GifImages));
                    imgs->w = width;
                    imgs->h = height;
                    imgs->size = width * height * 2;  /* use 16 bits color */
                }

                gif_add_image(imgs, gif, transp, p);
                free(p);

                break;
            }
                
            case EXTENSION_RECORD_TYPE:
            {
                GIF_CHECK_RETURN(DGifGetExtension(gif, &temp_save.Function, &extra) != GIF_ERROR);

                while (extra)
                {
                    /* Create an extension block with our data */
                    GIF_CHECK_RETURN(AddExtensionBlock(&temp_save, extra[0], &extra[1]) != GIF_ERROR);
                    GIF_CHECK_RETURN(DGifGetExtensionNext(gif, &extra) != GIF_ERROR);

                    temp_save.Function = 0;
                }
                break;
            }
                
            case TERMINATE_RECORD_TYPE:
                break;
                
            default: break; /* Should be trapped by DGifGetRecordType */
        }
    }
    while (type != TERMINATE_RECORD_TYPE);

DONE:
    DGifCloseFile(gif);
    fclose(fp);

    return imgs;
}

