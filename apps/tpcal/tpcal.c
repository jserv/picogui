/*
 * Touch-panel calibration program
 * Copyright (C) 1999 Bradley D. LaRonde <brad@ltc.com>
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contributors:
 *    Eric Christianson, RidgeRun, Inc. -
 *      converted to use picoGui libraries.
 *    Yann Vernier <yann@algonet.se> -
 *      pgserver changes to make tpcal useful, various other changes
 *    Micah Dowty <micahjd@users.sourceforge.net>
 *      updates for Input Filters support
 *     
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <netinet/in.h>		/* htonl */

#include <picogui.h>
#include "transform.h"
#include "calth.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static int xext=0, yext=0, xoffs=0, yoffs=0, rotation;
static CALIBRATION_PAIRS cps;
static CALIBRATION_PAIR* pcp = 0;
static POINT current_target_location, penposition, physicalresolution;
static TRANSFORMATION_COEFFICIENTS tc;

static int total_targets = 5;
static int current_target = 0;
const int inset = 10;

pghandle  wCanvas, infilter;
int calth;

POINT coord_logicalize(POINT pp)
 {
  POINT lp;

  switch(rotation)
   {
    case 0:
      lp=pp;
      break;
    case PG_VID_ROTATE90:
      lp.x=physicalresolution.y-1-pp.y;
      lp.y=pp.x;
      break;
    case PG_VID_ROTATE180:
      lp.x=physicalresolution.x-1-pp.x;
      lp.y=physicalresolution.y-1-pp.y;
      break;
    case PG_VID_ROTATE270:
      lp.x=pp.y;
      lp.y=physicalresolution.x-1-pp.x;
      break;
   }
  return lp;
 }

POINT coord_physicalize(POINT lp)
 {
  POINT pp;

  switch(rotation)
   {
    case 0:
      pp=lp;
      break;
    case PG_VID_ROTATE90:
      pp.x=lp.y;
      pp.y=physicalresolution.y-1-lp.x;
      break;
    case PG_VID_ROTATE180:
      pp.x=physicalresolution.x-1-lp.x;
      pp.y=physicalresolution.y-1-lp.y;
      break;
    case PG_VID_ROTATE270:
      pp.x=physicalresolution.x-1-lp.y;
      pp.y=lp.x;
      break;
   }
  return pp;
 }

POINT GetTarget(int n)
{
  POINT p;

  switch (n)
    {
    case 0:
      p.x = xext / 2; p.y = yext / 2;
      pcp = &cps.center;
      break;

    case 1:
      p.x = inset; p.y = inset;
      pcp = &cps.ul;
      break;

    case 2:
      p.x = xext - inset; p.y = inset;
      pcp = &cps.ur;
      break;

    case 3:
      p.x = xext - inset; p.y = yext - inset;
      pcp = &cps.lr;
      break;

    case 4:
      p.x = inset; p.y = yext - inset;
      pcp = &cps.ll;
      break;
		
    default:
      // return a random target
      p.x = random() / (RAND_MAX / xext);
      p.y = random() / (RAND_MAX / yext);
      pcp = 0;
      break;
    }
  return p;
}




void showTransformations(void)
{
  char str[256];
  union pg_client_trigger trig;

  /* Done with our input filter now */
  pgDelete(infilter);

  CalcTransformationCoefficientsBest(&cps.center, &tc, total_targets);
  sprintf(str, "COEFFv1 %d %d %d %d %d %d %d",
	 tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
  DBG(("%s\n",str));

  /* Send a new calibration to pgserver's calibration input filter */
  memset(&trig,0,sizeof(trig));
  trig.content.type = PG_TRIGGER_TS_CALIBRATE;
  trig.content.u.mouse.ts_calibration = pgNewString(str);
  pgInFilterSend(&trig);

#if 0
  pgMessageDialogFmt("Done!",0,
		     "You completed calibration!\n\n" 
		     "%s",str);
#endif
  pgFlushRequests();
  exit(0);
}

void DrawTarget(POINT p, unsigned long int c) {
  pgWriteCmd(wCanvas, PGCANVAS_GROP, 2, PG_GROP_SETCOLOR, c);
  pgWriteCmd(wCanvas, PGCANVAS_EXECFILL, 6, calth, TARGET, p.x, p.y, 1, 1);
  pgWriteCmd(wCanvas, PGCANVAS_REDRAW, 0);
  pgSubUpdate(wCanvas);
}

int evtBuild(struct pgEvent *evt) {
  xext = evt->e.size.w;
  yext = evt->e.size.h;
  xoffs = pgGetWidget(evt->from, PG_WP_ABSOLUTEX);
  yoffs = pgGetWidget(evt->from, PG_WP_ABSOLUTEY);
  pgWriteCmd(wCanvas, PGCANVAS_EXECFILL, 6, calth, PGTH_P_BGFILL,
      0, 0, xext, yext);
  current_target_location = GetTarget(current_target);
  DrawTarget(current_target_location, 0x000000);
  return 0;
}

int evtDrawTarget(struct pgEvent *evt) {
  DrawTarget(current_target_location, 0xD0D0D0);

  if (current_target != total_targets) {
    /* draw new target at current location */
    current_target_location = GetTarget(current_target);
    DrawTarget(current_target_location, 0x000000);
  }
  return 0;
}

int tpcalInFilter(struct pgEvent *evt) {
  union pg_client_trigger *trig = evt->e.data.trigger;
  POINT hit;
  int target;
  static int old_btn = 0;
  int btn;

  penposition.x = trig->content.u.mouse.x;
  penposition.y = trig->content.u.mouse.y;
  btn = trig->content.u.mouse.btn;
  if (btn == old_btn)
    return 0;
  old_btn = btn;

  DBG((__FUNCTION__ " (x=%d,y=%d,btn=%d)\n",penposition.x,penposition.y,btn));

  if (btn) {
    /* Stylus pressed, record a calibration point */

    if (pcp == 0)
      return 0;
    
    pcp->screen.x = current_target_location.x + xoffs;
    pcp->screen.y = current_target_location.y + yoffs;
    pcp->screen=coord_physicalize(pcp->screen);
    hit.x = pcp->device.x = penposition.x;
    hit.y = pcp->device.y = penposition.y;
    target=current_target++;

    DBG((__FUNCTION__ " screen(%d,%d) device(%d,%d)\n",pcp->screen.x,pcp->screen.y,
	 pcp->device.x,pcp->device.y));
    
    if(!CalcTransformationCoefficientsBest(&cps.center, &tc, current_target))
      {
	hit=pentoscreen(hit, &tc);
	DBG((__FUNCTION__ " hit(%d,%d)\n",hit.x,hit.y));
	coord_logicalize(hit);
	DBG((__FUNCTION__ " locigalized hit(%d,%d)\n",hit.x,hit.y));
	hit.x-=xoffs;
	hit.y-=yoffs;
	DrawTarget(hit, 0x808080);
      }
    DrawTarget(current_target_location, 0xD0D0D0);
    
    evtDrawTarget(evt);
  }
  else {
    /* Stylus released, complete transformation if we're done */

    if(current_target == total_targets)
      showTransformations();
  }

  return 0;
}

int main(int argc, char *argv[]) 
{
  struct pgmodeinfo mi;

  srandom(time(NULL));

  pgInit(argc,argv);

  mi=*pgGetVideoMode();
  physicalresolution.x=mi.xres;
  physicalresolution.y=mi.yres;

  /* Figure out the server's rotation
   */
  rotation = mi.flags & PG_VID_ROTATEMASK;
  switch (mi.flags & PG_VID_ROTBASEMASK) {

  case PG_VID_ROTBASE90:
    switch (rotation) {
    case 0:                rotation = PG_VID_ROTATE90;  break;
    case PG_VID_ROTATE90:  rotation = PG_VID_ROTATE180; break;
    case PG_VID_ROTATE180: rotation = PG_VID_ROTATE270; break;
    case PG_VID_ROTATE270: rotation = 0;                break;
    }
    break;

  case PG_VID_ROTBASE180:
    switch (rotation) {
    case 0:                rotation = PG_VID_ROTATE180; break;
    case PG_VID_ROTATE90:  rotation = PG_VID_ROTATE270; break;
    case PG_VID_ROTATE180: rotation = 0;                break;
    case PG_VID_ROTATE270: rotation = PG_VID_ROTATE90;  break;
    }
    break;

  case PG_VID_ROTBASE270:
    switch (rotation) {
    case 0:                rotation = PG_VID_ROTATE270; break;
    case PG_VID_ROTATE90:  rotation = 0;                break;
    case PG_VID_ROTATE180: rotation = PG_VID_ROTATE90;  break;
    case PG_VID_ROTATE270: rotation = PG_VID_ROTATE180; break;
    }
    break;
  }

  pgLoadTheme(pgFromFile("calth.th"));
  calth = pgFindThemeObject("tpcal");
  pgNewPopup(mi.lxres, mi.lyres);
  wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  pgSetWidget(0, PG_WP_THOBJ, calth, 0);
  pgBind(PGDEFAULT,PG_WE_BUILD,&evtBuild,NULL);

  /* Set up an input filter to recieve events before they get to the
   * touchscreen calibration input filter.
   * This filter gets a copy of mouse up/down events, and all
   * mouse-related events are absorbed.
   */
  infilter = pgNewInFilter(0,PG_TRIGGER_TOUCHSCREEN,PG_TRIGGER_TOUCHSCREEN);
  pgBind(PGBIND_ANY, PG_NWE_INFILTER, tpcalInFilter, NULL);

  pgEventLoop();

  return 0;
}

