/* $Id$
 *
 * mgl2fb.c - Video driver for MGL2
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 *               Hiroyuki Yanai <fk200329@fsinet.or.jp>
 * 
 * 
 * 
 */

/*
 * MGL -- MobileGear Graphic Library -
 * Copyright (C) 1998, 1999, 2000, 2001
 *      Koji Suzuki (suz@at.sakura.ne.jp)
 *      Yukihiko Sano (yukihiko@yk.rim.or.jp)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY KOJI SUZUKI AND YUKIHIKO SANO AND
 * CONTRIBUTORS``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <pgserver/common.h>

#ifdef DRIVER_MGL2FB

#include <pgserver/video.h>
#include <pgserver/input.h>

#include <mgl2.h>
#include <mglcol.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

static struct virtual_key *vk;
static int mc_plt[258];

g_error mgl2fb_init(void);
void mgl2fb_close(void);
void mgl2fb_update(hwrbitmap d,s16 x, s16 y, s16 w, s16 h);
g_error mgl2fb_regfunc(struct vidlib *v);


g_error mgl2fb_init(void) {
  g_error e;
  int r, g, b, i;

  FB_MEM = NULL;

  SCREEN_WIDTH = 640;
  SCREEN_HEIGHT = 480;

  if (!open_graph())
    return mkerror(PG_ERRT_IO, 46);

  vk = create_virtual_key3(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MK_V1, MK_V2, MK_V3);
  if (!vk)
    return mkerror(PG_ERRT_IO, 46);

  vk_attach(NULL, vk);

  e = g_malloc((void **) &FB_MEM, (SCREEN_WIDTH + 1) * (SCREEN_HEIGHT + 1));
  errorcheck;

  vid->xres = SCREEN_WIDTH;
  vid->yres = SCREEN_HEIGHT;
  vid->bpp = 8;
  FB_BPL = SCREEN_WIDTH;
  setvbl_linear8(vid);

  for (i = 0; i < 256; i++)
    mc_plt[i] = mc_from_rgb(packRGB(((i >> 4) & 0x0c), ((i >> 2) & 0x0e), ((i << 1) & 0x0e)));

#if 0
  mgl_set_keymode(MGL_SK_EXTRANSLATED);
#endif

  /* Load a main input driver */
  return load_inlib(&mgl2input_regfunc,&inlib_main);
}

void mgl2fb_close(void) {
  /* Free backbuffer */
  if (FB_MEM)
    g_free(FB_MEM);

  if (vk)
    vk_detach(vk, 1);

  unload_inlib(inlib_main);   /* Take out our input driver */
  close_graph();
}

void mgl2fb_update(hwrbitmap d,s16 x, s16 y, s16 w, s16 h) {
  u8 *srcptr, *ptr;
  int xx, xleft, i, j;
  int pixbuf[8];

  if (!FB_MEM)
    return;

#if 0
  srcptr = FB_MEM + y * FB_BPL + x;
  for (i = 0; i < h; i++) {
    xx = x;
    ptr = srcptr;
    for (j = 0; j < w; j++) {
      put_pixel(xx++, y, mc_plt[*ptr++]);
    }
    y++;
    srcptr += FB_BPL;
  }
#endif

  xleft = w % 8;
  srcptr = FB_MEM + y * FB_BPL + x;
  for (i = 0; i < h; i++) {
    xx = x;
    ptr = srcptr;
    for (j = 0; j < w; j += 8, xx += 8) {
      pixbuf[0] = mc_plt[*ptr++];
      pixbuf[1] = mc_plt[*ptr++];
      pixbuf[2] = mc_plt[*ptr++];
      pixbuf[3] = mc_plt[*ptr++];
      pixbuf[4] = mc_plt[*ptr++];
      pixbuf[5] = mc_plt[*ptr++];
      pixbuf[6] = mc_plt[*ptr++];
      pixbuf[7] = mc_plt[*ptr++];

      put_pixstream(xx, y, pixbuf, 8, DIR_NORTH);
    }

    for (j = 0; j < xleft; j++)
      put_pixel(xx++, y, mc_plt[*ptr++]);

    y++;
    srcptr += FB_BPL;
  }
}

g_error mgl2fb_regfunc(struct vidlib *v) {
  v->init = &mgl2fb_init;
  v->close = &mgl2fb_close;
  v->update = &mgl2fb_update;
  return success;
}

#endif /* DRIVER_MGL2FB */

/* The End */

