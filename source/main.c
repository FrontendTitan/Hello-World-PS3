/**
 * @file source/main.c
 * @brief This file contains the main function for the Hello World project.
 *
 *      Please do not touch the #include statements.
 * You will eventually need them
 * and it keep's the compiler happy.
 *      Make sure your dev env is setup correctly,
 * I will not help you with that. Of course I won't.
 * Just figure it out, it's not that hard.
 *      Good luck.
 */
#include <unistd.h>
#include <net/net.h>
#include <libfont.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <io/pad.h>
#include <sysmodule/sysmodule.h>
#include <pngdec/pngdec.h>
#include <tiny3d.h>

#include <ppu-lv2.h>
#include <rsx/rsx.h>
#include <rsx/commands.h>
#include <lv2/spu.h>
#include <lv2/memory.h>
#include <lv2/thread.h>
#include <sysutil/sysutil.h>
#include <rsx/commands.h>
#include <spurs/types.h>
#include <sysutil/video.h>
#include <lv2/cond.h>
#include <io/pad.h>
#include <lv2/mutex.h>
#include <lv2/systime.h>
#include <io/pad.h>
#include <lv2/syscalls.h>
#include <sysutil/msg.h>
#include <sysutil/sysutil.h>
#include <lv2/cond.h>
#include <lv2/mutex.h>

#include "dejavusans_ttf_bin.h"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

/******************************************************************************************************************************************************/
/* TTF functions to load and convert fonts                                                                                                             */
/******************************************************************************************************************************************************/

int ttf_inited = 0;

FT_Library freetype;
FT_Face face;

/* TTFLoadFont can load TTF fonts from device or from memory:

path = path to the font or NULL to work from memory

from_memory = pointer to the font in memory. It is ignored if path != NULL.

size_from_memory = size of the memory font. It is ignored if path != NULL.

*/

int TTFLoadFont(char *path, void *from_memory, int size_from_memory)
{

    if (!ttf_inited)
        FT_Init_FreeType(&freetype);
    ttf_inited = 1;

    if (path)
    {
        if (FT_New_Face(freetype, path, 0, &face))
            return -1;
    }
    else
    {
        if (FT_New_Memory_Face(freetype, from_memory, size_from_memory, 0, &face))
            return -1;
    }

    return 0;
}

void TTFUnloadFont()
{
    FT_Done_FreeType(freetype);
    ttf_inited = 0;
}

void TTF_to_Bitmap(u8 chr, u8 *bitmap, short *w, short *h, short *y_correction)
{
    FT_Set_Pixel_Sizes(face, (*w), (*h));

    FT_GlyphSlot slot = face->glyph;

    memset(bitmap, 0, (*w) * (*h));

    if (FT_Load_Char(face, (char)chr, FT_LOAD_RENDER))
    {
        (*w) = 0;
        return;
    }

    int n, m, ww;

    *y_correction = (*h) - 1 - slot->bitmap_top;

    ww = 0;

    for (n = 0; n < slot->bitmap.rows; n++)
    {
        for (m = 0; m < slot->bitmap.width; m++)
        {

            if (m >= (*w) || n >= (*h))
                continue;

            bitmap[m] = (u8)slot->bitmap.buffer[ww + m];
        }

        bitmap += *w;

        ww += slot->bitmap.width;
    }

    *w = ((slot->advance.x + 31) >> 6) + ((slot->bitmap_left < 0) ? -slot->bitmap_left : 0);
    *h = slot->bitmap.rows;
}

// draw one background color in virtual 2D coordinates
void DrawBackground2D(u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(0, 0, 65535);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(847, 0, 65535);

    tiny3d_VertexPos(847, 511, 65535);

    tiny3d_VertexPos(0, 511, 65535);
    tiny3d_End();
}

void drawScene()
{
    float x, y;

    tiny3d_Project2D(); // change to 2D context (remember you it works with 848 x 512 as virtual coordinates)

    DrawBackground2D(0x000000ff); // black with full opacity
    SetFontSize(18, 32);

    x = 0.0;
    y = 0.0;
    SetCurrentFont(1);
    SetFontColor(0xffffffff, 0x0);
    x = DrawString(x, y, "Hello, World!");

    y = GetFontY() + 40;
    x = 0;
    SetFontColor(0xff00ffff, 0x0);
    DrawString(x, y, "Press X to exit...");
}

void LoadTexture()
{
    u32 *texture_mem = tiny3d_AllocTexture(64 * 1024 * 1024); // alloc 64MB of space for textures (this pointer can be global)
    u32 *texture_pointer;                                     // use to asign texture space without changes texture_mem

    if (!texture_mem)
        return; // fail!

    texture_pointer = texture_mem;

    ResetFont();

    TTFLoadFont(NULL, (void *)dejavusans_ttf_bin, dejavusans_ttf_bin_size);
    texture_pointer = (u32 *)AddFontFromTTF((u8 *)texture_pointer, 32, 255, 32, 32, TTF_to_Bitmap);
    TTFUnloadFont();

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes
}

padInfo padinfo;
padData paddata;

s32 main(s32 argc, const char *argv[])
{

    int i;

    tiny3d_Init(1024 * 1024);

    ioPadInit(7);

    LoadTexture();

    // Ok, everything is setup. Now for the main loop.
    while (1)
    {
        // Check the pads.
        ioPadGetInfo(&padinfo);

        for (i = 0; i < MAX_PADS; i++)
        {
            if (padinfo.status[i])
            {
                ioPadGetData(i, &paddata);

                // Exit if you press X
                if (paddata.BTN_CROSS)
                {
                    return 0;
                }
            }
        }

        /* DRAWING STARTS HERE */
        // clear the screen, buffer Z and initializes environment to 2D
        tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);

        // Enable alpha Test
        tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

        // Enable alpha blending.
        tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
                         TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
                         TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);

        drawScene(); // Draw

        /* DRAWING FINISH HERE */

        tiny3d_Flip();
    }

    return 0;
}
