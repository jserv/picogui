/* $Id$
 *
 * mgl2input.c - input driver for MGL2
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

#ifdef DRIVER_MGL2INPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>

#include <mgl2.h>
#include <mglkey.h>

static int mx, my, mv, mbtn;

#define vkloc2pointing(state, btn) { \
 if (vk_x > 0 && vk_x < SCREEN_WIDTH) { \
   if (vk_y > 0 && vk_y < SCREEN_HEIGHT) { \
     mx = vk_x; \
     my = vk_y; \
   } \
 } \
 infilter_send_pointing((state), mx, my, (btn),NULL); \
}

void mgl2input_poll(void) {
  int key, modkey;

  key = get_key_im(0);
  if (key == -1)
    return;

  switch (key) {
  case MK_V1:
    vkloc2pointing(PG_TRIGGER_DOWN, 1);
    break;
  case MK_V2:
    vkloc2pointing(PG_TRIGGER_MOVE, 1);
    break;
  case MK_V3:
    vkloc2pointing(PG_TRIGGER_UP, 0);
    break;
  case MK_F1:
  case MK_LEFT:
    mx -= mv;
    if (mx < 0)
      mx = 0;
    infilter_send_pointing(PG_TRIGGER_MOVE, mx, my, 1,NULL);
    break;
  case MK_F2:
  case MK_DOWN:
    my += mv;
    if (my > SCREEN_HEIGHT)
      my = SCREEN_HEIGHT;
    infilter_send_pointing(PG_TRIGGER_MOVE, mx, my, 1,NULL);
    break;
  case MK_F3:
  case MK_UP:
    my -= mv;
    if (my < 0)
      my = 0;
    infilter_send_pointing(PG_TRIGGER_MOVE, mx, my, 1,NULL);
    break;
  case MK_F4:
  case MK_RIGHT:
    mx += mv;
    if (mx > SCREEN_WIDTH)
      mx = SCREEN_WIDTH;
    infilter_send_pointing(PG_TRIGGER_MOVE, mx, my, 1,NULL);
    break;
  case MK_F5:
    /*  case 0x20: */
    if (mbtn == 0) {
      infilter_send_pointing(PG_TRIGGER_DOWN, mx, my, 1,NULL);
      get_key(-1);
      mbtn = 1;
    } else {
      infilter_send_pointing(PG_TRIGGER_UP, mx, my, 0,NULL);
      get_key(-1);
      mbtn = 0;
    }
    break;
  case MK_F6:
  case MK_NUMLOCK:
    mv = (mv > 30) ? (1) : (mv + 5);
    break;
  default:
    modkey = 0;

#if 0
    if (key & MGL_SKM_CAPS)
      modkey = PGMOD_CAPS;
    else if (key & MGL_SKM_SHIFT) 
      modkey = PGMOD_SHIFT;
    else if (key & MGL_SKM_CTRL)
      modkey = PGMOD_CTRL;
    else if (key & MGL_SKM_ALT)
      modkey = PGMOD_ALT;

    infilter_send_key(PG_TRIGGER_CHAR, key & ~MGL_SKM_MASK, modkey);
#else
    infilter_send_key(PG_TRIGGER_CHAR, key, modkey);
#endif
    break;
  }
}

g_error mgl2input_init(void) {
  mx = SCREEN_WIDTH / 2;
  my = SCREEN_HEIGHT / 2;
  mv = 1;
  mbtn = 0;
  return success;
}

g_error mgl2input_regfunc(struct inlib *i) {
  i->init = &mgl2input_init;
  i->poll = &mgl2input_poll;
  return success;
}

#endif /* DRIVER_MGL2INPUT */

/* The End */

