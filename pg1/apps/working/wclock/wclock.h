/*
   Copyright (C) 2002 by Pascal Bauermeister

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with PocketBee; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Pascal Bauermeister
   Contributors:

   $Id$
*/

#ifndef __PGWCLOCK_PGWCLOCK_H__
#define __PGWCLOCK_PGWCLOCK_H__

#include <stdlib.h>
#include <stdio.h>
#include <picogui.h>

#include "debug.h"

/* ------------------------------------------------------------------------- */

extern pgcontext gfx_context;
extern pghandle  main_window;
extern int needs_pgfx_update;

static inline void set_color(pgcolor color)
{
  if(gfx_context) pgSetColor(gfx_context, color);
}

static inline void clear_bg()
{
  if(gfx_context) pgRect(gfx_context, 0, 0, 0x7FFF, 0x7FFF);
  if(gfx_context) pgContextUpdate(gfx_context);
}

static inline void rectangle(int x1, int y1, int w, int h)
{
  if(gfx_context) pgRect(gfx_context, x1, y1, w, h);
}

static inline void line(int x1, int y1, int x2, int y2)
{
  if(gfx_context) pgLine(gfx_context, x1, y1, x2, y2);
}

static inline void pixel(int x, int y)
{
  if(gfx_context) pgPixel(gfx_context, x, y);
}

static inline void pgfx_update(void)
{
  ENTER("pgfx_update()");
  static int first_time = 1;
  if(gfx_context) {
    if(!first_time) {
      pgContextUpdate(gfx_context);
    }
    first_time = 0;
  }
  LEAVE;
}

static inline void update(void)
{
  ENTER("update()");
  pgFlushRequests();
  if(needs_pgfx_update) {
    pgfx_update();
    needs_pgfx_update = 0;
  }
  pgUpdate();
  LEAVE;
}

/* ------------------------------------------------------------------------- */

/* Constants */
enum {
  /* misc */
  NB_ZONES       = 30,
  TIMEBAR_HEIGHT = 20,

  /* colors */
  BLACK   = 0x000000,
  WHITE   = 0xffffff,
  FLAT    = 0xc0c0c0,
  ILLUM   = FLAT + 0x101010 * 2,
  SHADOW  = FLAT - 0x101010 * 2,
};

extern void init_zones();
extern pghandle create_world_bitmap();

extern void get_delta(int zone, int* sign, int* hour, int* min);
extern void get_world_size(int* w, int* h);
extern int  get_world_width(void);
extern int  get_world_height(void);

extern void draw_zone(int nr);
extern void draw_timebar(int gmt_h, int current_nr, int with_text);

/* ------------------------------------------------------------------------- */

#endif
