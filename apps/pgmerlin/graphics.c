/***************************************************************************
 * graphics.c
 * 
 * PicoGUI:
 * Copyright (c) 2002 bigthor.
 * Email: bigthor@softhome.net
 *
 * X Window:
 * Copyright (c) 2000 Stefan Hellkvist, Ericsson Radio Systems AB 
 * Email: stefan@hellkvist.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *****************************************************************************/

#include "graphics.h"
#include "pattern.h"
#include "event.h"
#include <string.h>
#include <stdio.h>
/* pico-includes */
#include <picogui.h>
#include <string.h>
/* end pico-includes */

unsigned long getColor( Display *dsp, unsigned short red, unsigned green, unsigned blue );
/* pico-funcs */
void makeColorSample(pghandle canvas, pgcolor color);
void setCurrentColor(pgcolor color);
void clearBackground(pgcolor color);
void drawPenLine(pgcontext gc, pgu x1,pgu y1,pgu x2,pgu y2);
void updateAppTitle(void);
int evtColorSample(struct pgEvent *evt);
int evtCanvas(struct pgEvent *evt);
int evtClose(struct pgEvent *evt);
/* end pico-funcs */

Display *myDisplay;
Window myWindow;
GC myGC;
int myScreen;
int window_x, window_y, window_width, window_height;
/* pico-vars */
pghandle wCurrentColor, wMainCanvas, bScribbleBitmap, wApp;
pgcontext gcCurrentColor, gcMain, gcBitmap;
pgcolor currentColor;
int needs_save;
const char *currentFile = NULL;
/* end pico-vars*/

extern Pattern *current;
XFontStruct *myFont;

/* pico-defs */
#define APP_NAME "pgmerlin"
/* end pico-defs */


void
assignDefault()
{
    window_x = window_y = 0;
    window_width = DEFAULT_WINDOW_WIDTH;
    window_height = DEFAULT_WINDOW_HEIGHT;
}

int
parseArgs( int argc, char *argv[])
{
    char xsign, ysign;
    if ( argc != 3 || strcmp( "-geometry", argv[1] ) )
	return 1;

    if ( sscanf( argv[2], "%dx%d%c%d%c%d\n", &window_width, 
		 &window_height, 
		 &xsign, &window_x, &ysign, &window_y ) != 6 ) 
    	return 1;

/*      if ( xsign == '-' ) */
/*      { */
/*  	int screen_width = WidthOfScreen(  */
/*  	    ScreenOfDisplay( myDisplay, myScreen ) ); */
/*  	window_x = screen_width - window_width - window_x; */
/*      } */

/*      if ( ysign == '-' ) */
/*      { */
/*  	int screen_height = HeightOfScreen(  */
/*  	    ScreenOfDisplay( myDisplay, myScreen ) ); */
/*  	window_y = screen_height - window_height - window_y; */
/*      } */
    return 0;
}

/******************************************** Event Handlers */


void 
paintPattern()
{
/*      int i; */
/*      if ( current != NULL && current->nPoints > 1 ) */
/*      { */
/*  	for ( i = 0; i < current->nPoints - 1; i++ ) */
/*  	    if ( !current->pv[i].isLast )  */
/*  		XDrawLine( myDisplay, myWindow, myGC,  */
/*  			   current->pv[i].x, current->pv[i].y, */
/*  			   current->pv[i+1].x, current->pv[i+1].y ); */
/*      } */
}

void
paint( void )
{
/*      XClearWindow( myDisplay, myWindow ); */
/*      XSetLineAttributes( myDisplay, myGC, 1, LineOnOffDash,  */
/*  			CapNotLast, JoinMiter ); */
/*      XDrawLine( myDisplay, myWindow, myGC, window_width / 2, */
/*  	       20, window_width / 2, window_height ); */
/*      XSetLineAttributes( myDisplay, myGC, 1, LineSolid,  */
/*  			CapNotLast, JoinMiter ); */
/*      paintPattern(); */
  clearBackground(0xffffff);
}


/******************************************** Main Program */

void initGraphics(int argc, char **argv) {
  pghandle wColorTB,wMenuTB;
  int i;
  struct pgmodeinfo mi;

  if(parseArgs(argc, argv))
    assignDefault();

  pgInit(argc,argv);

  wApp = pgRegisterApp(PG_APP_NORMAL,APP_NAME,0);
  pgBind(PGDEFAULT,PG_WE_CLOSE,&evtClose,NULL);

  /* Get info about the current video mode- we use this to figure out
   * what size the bitmap should be, and what type of palette to use.
   */
  mi = *pgGetVideoMode();

  /* Top-level widgets- color toolbar and main canvas */
/*    wColorTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0); */
/*    pgSetWidget(PGDEFAULT, */
/*  	      PG_WP_SIDE,PG_S_BOTTOM, */
/*  	      0); */
/*    wMenuTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0); */
/*    pgSetWidget(PGDEFAULT, */
/*  	      PG_WP_SIDE,PG_S_TOP, */
/*  	      0); */
  wMainCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  pgBind(PGDEFAULT,PGBIND_ANY,&evtCanvas,NULL);

  /* The canvas does not by default process mouse movement events. Turn this
   * on using PG_WP_TRIGGERMASK
   */
  pgSetWidget(wMainCanvas,
	      PG_WP_TRIGGERMASK,pgGetWidget(wMainCanvas,
					    PG_WP_TRIGGERMASK) | PG_TRIGGER_MOVE,
	      0);

  /* Allocate a bitmap for the drawing. Since we don't have scrolling,
   * it is safe to make the bitmap the size of the display.
   * When we draw, it goes to this bitmap and to the canvas. This way it is
   * drawn quickly, but if PicoGUI needs to repaint the screen it can use
   * the bitmap. Also, this will make loading and saving drawings very easy.
   */
  bScribbleBitmap = pgCreateBitmap(mi.lxres, mi.lyres);
  gcBitmap = pgNewBitmapContext(bScribbleBitmap);

  /* Use a persistent context to make the canvas automatically draw
   * bScribbleBitmap when it needs a redraw
   */
  gcMain = pgNewCanvasContext(wMainCanvas,PGFX_PERSISTENT);
  pgBitmap(gcMain,0,0,mi.lxres,mi.lyres,bScribbleBitmap);
  pgDeleteContext(gcMain);

  /* Make an immediate-mode context for the canvas now */
  gcMain = pgNewCanvasContext(wMainCanvas,PGFX_IMMEDIATE);

  /* Make the 'current color' box on the left */
/*    wCurrentColor = pgNewWidget(PG_WIDGET_CANVAS,PG_DERIVE_INSIDE,wColorTB); */
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  gcCurrentColor = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);
      
  /* If it's a big display and in color, give them a nice palette */
/*    if (mi.bpp >= 8 && mi.lxres >= 320) { */
/*      for (i=15;i>=0;i--) { */
/*        if (i==8) */
/*  	continue; */
/*        pgNewWidget(PG_WIDGET_CANVAS,0,0); */
/*        pgSetWidget(PGDEFAULT, */
/*  		  PG_WP_SIDE,PG_S_RIGHT, */
/*  		  0); */
        /* 15-color EGA-like palette */ 
/*        makeColorSample(PGDEFAULT, */
/*  		    (i & 0x08) ? */
/*  		      (((i & 0x04) ? 0xFF0000 : 0) | */
/*  		       ((i & 0x02) ? 0x00FF00 : 0) | */
/*  		       ((i & 0x01) ? 0x0000FF : 0)) : */
/*  		      (((i & 0x04) ? 0x800000 : 0) | */
/*  		       ((i & 0x02) ? 0x008000 : 0) | */
/*  		       ((i & 0x01) ? 0x000080 : 0))); */
/*      } */
/*    } */
    /* Draw a simple grayscale palette */ 
/*    else { */
/*      pgNewWidget(PG_WIDGET_CANVAS,0,0); */
/*      pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0); */
/*      makeColorSample(PGDEFAULT,0xFFFFFF); */
/*      pgNewWidget(PG_WIDGET_CANVAS,0,0); */
/*      pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0); */
/*      makeColorSample(PGDEFAULT,0xAAAAAA); */

  /* FIXME: without this doesn't refresh */
    pgNewWidget(PG_WIDGET_CANVAS,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0);
    makeColorSample(PGDEFAULT,0x000000);
/*    } */
 
  setCurrentColor(0x000000);   /* Default foreground and background */
  clearBackground(0xFFFFFF);
  updateAppTitle();

  return 0;
}

/******************************************** Event Handlers */

int evtColorSample(struct pgEvent *evt) {
  setCurrentColor((pgcolor) evt->extra);
  return 0;
}

int evtCanvas(struct pgEvent *evt) {
  static pgu old_x = 0, old_y = 0;
  static int down;

  /* NORMAL Frame */
  pgWriteCmd(evt->from,PGCANVAS_GROP,6,PG_GROP_FRAME,
	     window_x,window_y,window_width/2,window_height,0x808080);
  /* NUMERIC Frame */
  pgWriteCmd(evt->from,PGCANVAS_GROP,6,PG_GROP_FRAME,
	     window_x+window_width/2,window_y,window_width/2,window_height,0x808080);

  switch (evt->type) {
    
  case PG_WE_PNTR_DOWN:
    down = 1;
    handleEvent( evt->e.pntr.x, evt->e.pntr.y, PEN_DOWN );
    old_x = evt->e.pntr.x;
    old_y = evt->e.pntr.y;
    break;

  case PG_WE_PNTR_UP:
  case PG_WE_PNTR_RELEASE:
    down = 0;
    handleEvent( evt->e.pntr.x, evt->e.pntr.y, PEN_UP );
  case PG_WE_PNTR_MOVE:
    if (!down)
      break;

    /* Draw on the screen and on the bitmap */
    drawPenLine(gcMain,old_x,old_y,evt->e.pntr.x,evt->e.pntr.y);
    drawPenLine(gcBitmap,old_x,old_y,evt->e.pntr.x,evt->e.pntr.y);
    handleEvent( evt->e.pntr.x, evt->e.pntr.y, PEN_DRAG );
    old_x = evt->e.pntr.x;
    old_y = evt->e.pntr.y;
    break;

  }
  return 0;
}

/* If they try to close with out saving, ask first */
int evtClose(struct pgEvent *evt) {
  switch (pgMessageDialog(APP_NAME,"Exit?",
			  PG_MSGBTN_YES | PG_MSGBTN_NO )) {
    
  case PG_MSGBTN_NO:       
    return 1;

  case PG_MSGBTN_YES:      
    return 0;
  }
  return 1;                /* Shouldn't get here */
}

/******************************************** Utilities */

/* Turns the given widget into a color sample, it changes the current color
 * when clicked.
 */
void makeColorSample(pghandle canvas, pgcolor color) {
  pgu btn_h,btn_s;
  pgcontext gc = pgNewCanvasContext(canvas,PGFX_PERSISTENT);

  /* Make the rectangle the same size and spacing as a button */
  btn_h = pgThemeLookup(PGTH_O_BUTTON,PGTH_P_HEIGHT);
  btn_s = pgThemeLookup(PGTH_O_BUTTON,PGTH_P_SPACING);
  pgSetColor(gc,0x000000);
  pgFrame(gc,btn_s,0,btn_h,btn_h);
  pgSetColor(gc,color);
  pgRect(gc,btn_s+2,2,btn_h-4,btn_h-4);
  
  pgDeleteContext(gc);
  pgBind(canvas,PG_WE_PNTR_DOWN,&evtColorSample,(void*) color);
}

void setCurrentColor(pgcolor color) {
  pgu w,h;

  currentColor = color;

  /* Clear the existing data (Remember, in persistant mode pgserver
   * remembers all drawing commands so it can automatically redraw things.
   * This clears all the drawing commands it has remembered for this canvas)
   */
  pgWriteCmd(wCurrentColor,PGCANVAS_NUKE,0);

  /* Draw the color sample */
  h = pgThemeLookup(PGTH_O_BUTTON,PGTH_P_HEIGHT);
  w = 3*h;
  pgSetColor(gcCurrentColor,0x000000);
  pgFrame(gcCurrentColor,0,0,w,h);
  pgSetColor(gcCurrentColor,currentColor);
  pgRect(gcCurrentColor,2,2,w-4,h-4);
  pgContextUpdate(gcCurrentColor);
}

void drawPenLine(pgcontext gc, pgu x1,pgu y1,pgu x2,pgu y2) {
  if (!needs_save) {
    needs_save = 1;
    updateAppTitle();
  }

  /* Draw multiple lines to make a thick line */
  pgSetColor(gc,currentColor);
  pgLine(gc,x1  ,y1  ,x2  ,y2  );
  pgLine(gc,x1+1,y1  ,x2+1,y2  );
  pgLine(gc,x1-1,y1  ,x2-1,y2  );
  pgLine(gc,x1  ,y1+1,x2  ,y2+1);
  pgLine(gc,x1  ,y1-1,x2  ,y2-1);

  pgContextUpdate(gc);
}

void clearBackground(pgcolor color) {
  pgSetColor(gcMain,color);
  pgRect(gcMain,0,0,0x7FFF,0x7FFF);
  pgSetColor(gcBitmap,color);
  pgRect(gcBitmap,0,0,0x7FFF,0x7FFF);
  pgContextUpdate(gcMain);
}

void updateAppTitle(void) {
  pgReplaceTextFmt(wApp,"%s",APP_NAME);
  pgSubUpdate(wApp);
}
