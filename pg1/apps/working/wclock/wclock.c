/*
   Copyright (C) 2002 by Pascal Bauermeister

   wclock (WorldClock):
   A PicoGUI application displaying most time zones on a world map.

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

#include <time.h>

#define DECLARE_DEBUG_VARS /* only in this file */
#include "wclock.h"

#define IDLE_TIME     500

pgcontext gfx_context = 0;
pghandle  main_window = 0;

static pghandle world_bitmap;
static pghandle ttl_text;
static int width, height;
static int current_zone = 0;
static int zone_to_refresh = -1;

static char zone_info[50] = "";
static time_t delta = 0;
static int gmt_h = -1;

enum { btnid_set, btnid_left, btnid_right };

static void idle_handler(void);

/*****************************************************************************/

static void
request_refresh(int zone)
{
  ENTER("request_refresh(int zone)");
  zone_to_refresh = zone;
  pgSetIdle(1, &idle_handler);
  LEAVE;
}

/*****************************************************************************/

static int
update_time(int zone)
{
  ENTER("update_time(int zone)");
  time_t now, then;
  struct tm *ltime;
  char buf[200];
  char fmt[100];
  int gh; 

  time(&now);
  ltime = localtime(&now);
  gh = ltime->tm_hour;
  then = now + delta;

  ltime = localtime(&then);

  sprintf(fmt, "%%l:%%M:%%S %%p %%b %%e  %s", zone_info); 
  strftime(buf, sizeof(buf), fmt, ltime);
  DPRINTF(">>>> [%s] <<<\n", buf);

  pgReplaceText(ttl_text, buf);

  if(gmt_h!=gh) {
    draw_timebar(gh, zone, 1);
    gmt_h = gh;
  }

  LEAVE;
  return 1;
}

/*****************************************************************************/

static int
redraw_world(int zone)
{
  ENTER("redraw_world(int zone)");
  int must_update = 0;
    
  /* draw the world */
  pgSetLgop(gfx_context, PG_LGOP_NONE);
  pgWriteCmd(main_window,
	     PGCANVAS_GROP, 6,
	     PG_GROP_BITMAP, 0, 0, width, height, world_bitmap);
  
  /* draw a zone */
  if(zone>=0) {
    set_color(0x505050);
    pgSetLgop(gfx_context, PG_LGOP_ADD);
    draw_zone(zone);
    pgSetLgop(gfx_context, PG_LGOP_NONE);
    must_update = 1;
  }
  
  LEAVE;
  return must_update;
}

/*****************************************************************************/

static void
update_info(int zone)
{
  ENTER("update_info(int zone)");
  int s, h, m;

  get_delta(zone, &s, &h, &m);
  delta = h*60*60 + m*60;
  if(s==-1) delta = -delta;

  sprintf(zone_info,
	  delta==0 ? "[GMT]" : m ? "[GMT%c%d:%02d]" : "[GMT%c%d]",
	  s==-1 ? '-' : s==1 ? '+' : '?',
	  h, m);

  LEAVE;
}

/*****************************************************************************/

static void
redraw_all(void)
{
  ENTER("redraw_all()");
  int must_update = 0;

  if(zone_to_refresh!=-1) {
    gmt_h = -1; /* force redraw time bar */
    update_info(zone_to_refresh);
    must_update |= redraw_world(zone_to_refresh);
    must_update |= update_time(zone_to_refresh);
    current_zone = zone_to_refresh;
    zone_to_refresh = -1;
  }
  else {
    must_update |= update_time(current_zone);
  }

  if(must_update) {
    update();
  }
  
  LEAVE;
}

/*****************************************************************************/

static void
idle_handler(void)
{
  ENTER("idle_handler(void)");

  pgSetIdle(IDLE_TIME, NULL);
  redraw_all();
  pgSetIdle(IDLE_TIME, &idle_handler);

  LEAVE;
}

/*****************************************************************************/

static int
btn_handler(struct pgEvent *evt)
{
  ENTER("btn_handler(struct pgEvent *evt)");
  int changed = 0;

  switch ((int)evt->extra) {

  case btnid_set:
    break;

  case btnid_left:
    changed = 1;
    --current_zone;
    if(current_zone<0) current_zone = NB_ZONES-1;
    break;

  case btnid_right:
    changed = 1;    
    ++current_zone;
    if(current_zone>=NB_ZONES) current_zone = 0;
    break;

  }

  if(changed) {
    request_refresh(current_zone);
  }
  
  LEAVE;
  return 1;
}


/*****************************************************************************/

static int
canvas_handler(struct pgEvent *evt)
{
  ENTER("canvas_handler(struct pgEvent *evt)");
  static int draw = 0;

  switch (evt->type) {

  case PG_WE_PNTR_DOWN:
    draw = 1;
    DPRINTF("PG_WE_PNTR_DOWN\n");
    break;

  case PG_WE_PNTR_UP:
    draw = 0;
    DPRINTF("PG_WE_PNTR_UP\n");
    break;

  case PG_WE_PNTR_RELEASE:
    draw = 0;
    DPRINTF("PG_WE_PNTR_RELEASE\n");
    break;

  case PG_WE_PNTR_MOVE:
    draw = 0;
    DPRINTF("PG_WE_PNTR_MOVE\n");
    break;

  case PG_WE_BUILD:
    DPRINTF("PG_WE_BUILD\n");
    request_refresh(current_zone);
    break;

  case PG_WE_KBD_KEYUP:
    DPRINTF("PG_WE_KBD_KEYUP\n");
    LEAVE;
    return 1;

  case PG_WE_KBD_KEYDOWN:
    DPRINTF("PG_WE_KBD_KEYDOWN\n");
    LEAVE;
    return 1;


  default:
    DPRINTF("??? unhandled type: 0x%x\n", evt->type);
    break;

  }

  if(draw) {
    int x = evt->e.pntr.x;
    int y = evt->e.pntr.y;
    int z = get_zone(x,y);
    DPRINTF("(%d,%d)", x, y);
    DPRINTF("=> z=%d\n", z);
    if(z>=0 && z!=current_zone) {
      current_zone = z;
      request_refresh(z);
    }
  }

  LEAVE;
  return 1;
}

/*****************************************************************************/

int
main(int argc, char** argv)
{
  pgcolor color;
  struct pgmodeinfo mi;
  int i;
  pghandle ttl_box;

  /* init PicoGUI client */
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL, argv[0], 0);

  /* create the title */
  ttl_box = pgNewWidget(PG_WIDGET_BOX,0,0);

  ttl_text = pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, ttl_box);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString("wclock"),
	       0);

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, ttl_box);
  pgSetWidget (PGDEFAULT,
               PG_WP_SIDE, PG_S_RIGHT,
 	       PG_WP_TEXT, pgNewString("?"),
               PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN,
               0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &btn_handler, (void*)btnid_set);

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, ttl_box);
  pgSetWidget (PGDEFAULT,
               PG_WP_SIDE, PG_S_LEFT,
 	       PG_WP_TEXT, pgNewString(">"),
               PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN,
               0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &btn_handler, (void*)btnid_right);

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, ttl_box);
  pgSetWidget (PGDEFAULT,
               PG_WP_SIDE, PG_S_LEFT,
 	       PG_WP_TEXT, pgNewString("<"),
               PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN,
               0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &btn_handler, (void*)btnid_left);

  /* create the main context */
  main_window = pgNewWidget(PG_WIDGET_CANVAS, PG_DERIVE_AFTER, ttl_box);
  pgBind(PGDEFAULT, PGBIND_ANY, &canvas_handler, NULL);

  /* init time zones */
  init_zones();

  /* activate mouse move, keyboard and focus events */
  pgSetWidget(main_window,
	      PG_WP_TRIGGERMASK, pgGetWidget(main_window, PG_WP_TRIGGERMASK) |
	      PG_TRIGGER_MOVE |
	      PG_TRIGGER_CHAR |
	      PG_TRIGGER_KEYDOWN |
	      PG_TRIGGER_KEYUP /* |
	      PG_TRIGGER_ACTIVATE |
	      PG_TRIGGER_DEACTIVATE */,
	      0);

  mi = *pgGetVideoMode();
  width = mi.lxres;
  height = mi.lyres;

  /* Make a backbuffer bitmap */
  world_bitmap = create_world_bitmap();
  width = get_world_width();
  height = get_world_height();

  /* Set the clipping rectangle */
  pgWriteCmd(main_window,
	     PGCANVAS_GROP, 5,
	     PG_GROP_SETCLIP, 0, 0, width, height + TIMEBAR_HEIGHT);
  
  /* Set to be always rendered */
  pgWriteCmd(main_window,
	     PGCANVAS_GROPFLAGS, 1,
	     PG_GROPF_UNIVERSAL);
  
  /* Create contexts for the canvas itself and the back-buffer */
  gfx_context = pgNewCanvasContext(main_window, PGFX_IMMEDIATE );

  /* run all */
  DPRINTF(">>> entering loop\n");
  pgEventLoop();

  return 0;
}

/*****************************************************************************/
