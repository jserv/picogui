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

#define DEBUG
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

pghandle  wCanvas;
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

  DBG((__FUNCTION__ " %d = (%d,%d)\n",n,p.x,p.y));

  return p;
}




void showTransformations(void)
{
  char str[256];

#if 0
  CalcTransformationCoefficientsSimple(&cps, &tc);
  printf("%d %d %d %d %d %d %d\n",
	 tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
  CalcTransformationCoefficientsBetter(&cps, &tc);
  printf("%d %d %d %d %d %d %d\n",
	 tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
  CalcTransformationCoefficientsEvenBetter(&cps, &tc);
  printf("%d %d %d %d %d %d %d\n",
	 tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
#endif
  CalcTransformationCoefficientsBest(&cps.center, &tc, total_targets);
  sprintf(str, "COEFFv1 %d %d %d %d %d %d %d",
	 tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
  DBG(("%s\n", str));
  pgDriverMessage(PGDM_INPUT_SETCAL, pgNewString(str));

  pgMessageDialogFmt("Done!",0,
		     "You completed calibration!\n\n" 
		     "%d %d %d %d %d %d %d\n",
		     tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);

  exit(0);
 }

void DrawTarget(POINT p, unsigned long int c) {
  DBG((__FUNCTION__" (%d,%d)\n",p.x, p.y));

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

  DBG((__FUNCTION__" current_target=%d (%d,%d)\n",current_target,xext,yext));

  if (current_target != total_targets) {
    /* draw new target at current location */
    current_target_location = GetTarget(current_target);
    DrawTarget(current_target_location, 0x000000);
  }
  return 0;
}

int evtPenUp(struct pgEvent *evt) {
  if(current_target == total_targets) {
    pgUnregisterOwner(PG_OWN_POINTER);
    pgDriverMessage(PGDM_INPUT_CALEN, 0);
    pgBind(PGBIND_ANY,PG_NWE_PNTR_DOWN,NULL,NULL);
    pgBind(PGBIND_ANY,PG_NWE_PNTR_UP,NULL,NULL);
    showTransformations();
  }
  return 0;
}

int evtPenPos(struct pgEvent *evt) {
  struct penposdata {s32 x, y; unsigned char r;} *data=
    (struct penposdata*)evt->e.data.pointer;

  if(evt->e.data.size!=9)
   {
    fprintf(stderr, "Penpos packet size %ld, expected 9\n", evt->e.data.size);
    pgDriverMessage(PGDM_INPUT_CALEN, 0);
    exit(1);
   }
  penposition.x=htonl(data->x);
  penposition.y=htonl(data->y);
  rotation=((unsigned char*)evt->e.data.pointer)[8];
  return 0;
}

int evtPenDown(struct pgEvent *evt) {
  POINT hit;
  int target;

  DBG((__FUNCTION__ " (x=%d,y=%d)\n",penposition.x,penposition.y));

  if (pcp == 0)
    return 0;

  pcp->screen.x = current_target_location.x + xoffs;
  pcp->screen.y = current_target_location.y + yoffs;
  pcp->screen=coord_physicalize(pcp->screen);
  hit.x = pcp->device.x = penposition.x;
  hit.y = pcp->device.y = penposition.y;
  target=current_target++;

  if(!CalcTransformationCoefficientsBest(&cps.center, &tc, current_target))
   {
    DBG(("Current target: %d\n", current_target));
    hit=pentoscreen(hit, &tc);
    coord_logicalize(hit);
    hit.x-=xoffs;
    hit.y-=yoffs;
    DrawTarget(hit, 0x808080);
   }
  DrawTarget(current_target_location, 0xD0D0D0);

  evtDrawTarget(evt);

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

  pgNewPopup(mi.xres, mi.yres);

  pgLoadTheme(pgFromFile("calth.th"));
  calth = pgFindThemeObject("tpcal");
  wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  pgSetWidget(0, PG_WP_THOBJ, calth, 0);
  pgBind(PGDEFAULT,PG_WE_BUILD,&evtBuild,NULL);
  pgBind(PGBIND_ANY,PG_NWE_PNTR_DOWN,&evtPenDown,NULL);
  pgBind(PGBIND_ANY,PG_NWE_PNTR_UP,&evtPenUp,NULL);
  pgBind(PGBIND_ANY,PG_NWE_CALIB_PENPOS,&evtPenPos,NULL);
  pgRegisterOwner(PG_OWN_POINTER);
  pgDriverMessage(PGDM_INPUT_CALEN, 1);

  pgEventLoop();

  return 0;
}

