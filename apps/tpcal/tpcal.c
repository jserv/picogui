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
 *     
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <picogui.h>
#include "transform.h"

#define DEBUG
#ifdef DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static int xext=0, yext=0;
static CALIBRATION_PAIRS cps;
static CALIBRATION_PAIR* pcp = 0;
static POINT current_target_location;

static int total_targets = 5;
static int current_target = 0;
const int inset = 10;

pghandle  wCanvas;
pghandle  wStatusLabel;


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

  TRANSFORMATION_COEFFICIENTS tc;
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
  CalcTransformationCoefficientsBest(&cps, &tc);
  printf("%d %d %d %d %d %d %d\n",
	 tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
    

  pgMessageDialogFmt("Done!",0,
		     "You completed calibration!\n\n" 
		     "%d %d %d %d %d %d %d\n",
		     tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);

  exit(0);

}

void DrawTarget(struct pgEvent *evt, POINT p, unsigned long int c) {
  const int center = 3;
  const int scale = 9;
  pgcontext gc;

  DBG((__FUNCTION__" (%d,%d)\n",p.x, p.y));

  /* I wanted to use a label to update new coordinates, didn't get this working yet. */
  pgReplaceTextFmt(wStatusLabel,"Please touch the center of the target (%d,%d)",p.x,p.y);

  gc = pgNewCanvasContext(evt->from,PGFX_PERSISTENT);

  pgSetColor(gc,c);        /* An 'X' across the canvas */
  pgFrame(gc,p.x-center,p.y-center,center*2+1,center*2+1);

  pgMoveTo(gc,p.x,p.y-scale);
  pgLineTo(gc,p.x,p.y+scale);

  pgMoveTo(gc,p.x-scale,p.y);
  pgLineTo(gc,p.x+scale,p.y);

  /* Draw it */
  pgContextUpdate(gc);  
  pgDeleteContext(gc);                                 

}

/* Redraw the game board */
int evtDrawTarget(struct pgEvent *evt) {

  POINT last = current_target_location;

  if(!xext) {
    xext = evt->e.size.w;
    yext = evt->e.size.h;
  } else {
    /* erase the old target */
    DrawTarget(evt, last, 0xFFFFFF);
  }

  DBG((__FUNCTION__" current_target=%d (%dx%d)\n",current_target,xext,yext));

  if (current_target == total_targets) {
    /* we are done, bring up a dialog and show the transformations */
    showTransformations();
  }

  /* draw new target at current location */
  current_target_location = GetTarget(current_target);
  DrawTarget(evt, current_target_location, 0x000000);

  return 0;

}

int evtMouseDown(struct pgEvent *evt) {

  DBG((__FUNCTION__ " (x=%d,y=%d)\n",evt->e.pntr.x,evt->e.pntr.y));

  if (pcp != 0) {
    pcp->screen.x = current_target_location.x * TRANSFORMATION_UNITS_PER_PIXEL;
    pcp->screen.y = current_target_location.y * TRANSFORMATION_UNITS_PER_PIXEL;
    pcp->device.x = evt->e.pntr.x;
    pcp->device.y = evt->e.pntr.y;
  }

  current_target++;

  evtDrawTarget(evt);

  return 0;
}

int main(int argc, char *argv[]) 
{
  pghandle fntLabel;

  srandom(time(NULL));

  pgInit(argc,argv);

  pgRegisterApp(PG_APP_NORMAL,"Please touch the center of the target.",0);

  fntLabel = pgNewFont(NULL,10,PG_FSTYLE_FIXED);


  wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  pgBind(PGDEFAULT,PG_WE_BUILD,&evtDrawTarget,NULL);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&evtMouseDown,NULL);

  /* couldn't figure out how to attach the label to the app bar, and have it replace text */
  wStatusLabel = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Please touch the center of the target"),
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TRANSPARENT,0,
	      PG_WP_ALIGN,PG_A_CENTER,
	      PG_WP_FONT,fntLabel,
	      0);

  pgEventLoop();

  return 0;
}



