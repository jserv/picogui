/* $Id$
 *
 * scribble.c - Simple paint program
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * 
 * 
 * 
 */

#include <picogui.h>
#include <string.h>

pghandle wCurrentColor, wMainCanvas, bScribbleBitmap, wApp;
pgcontext gcCurrentColor, gcMain, gcBitmap;
pgcolor currentColor;
int needs_save;
const char *currentFile = NULL;

void makeColorSample(pghandle canvas, pgcolor color);
void setCurrentColor(pgcolor color);
void clearBackground(pgcolor color);
void drawPenLine(pgcontext gc, pgu x1,pgu y1,pgu x2,pgu y2);
void updateAppTitle(void);
int evtColorSample(struct pgEvent *evt);
int evtBtnOther(struct pgEvent *evt);
int evtBtnClear(struct pgEvent *evt);
int evtBtnLoad(struct pgEvent *evt);
int evtBtnSave(struct pgEvent *evt);
int evtBtnSaveAs(struct pgEvent *evt);
int evtCanvas(struct pgEvent *evt);
int evtClose(struct pgEvent *evt);

#define APP_NAME "Scribble Pad"

/******************************************** Main Program */

int main(int argc, char **argv) {
  pghandle wColorTB,wMenuTB;
  int i;
  struct pgmodeinfo mi;

  pgInit(argc,argv);
  wApp = pgRegisterApp(PG_APP_NORMAL,APP_NAME,0);
  pgBind(PGDEFAULT,PG_WE_CLOSE,&evtClose,NULL);

  /* Get info about the current video mode- we use this to figure out
   * what size the bitmap should be, and what type of palette to use.
   */
  mi = *pgGetVideoMode();

  /* Top-level widgets- color toolbar and main canvas */
  wColorTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);
  wMenuTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_TOP,
	      0);
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
  wCurrentColor = pgNewWidget(PG_WIDGET_CANVAS,PG_DERIVE_INSIDE,wColorTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  gcCurrentColor = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);
  
  /* Add the 'Other...' button */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString("Other..."),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtnOther,NULL);
    
  /* If it's a big display and in color, give them a nice palette */
  if (mi.bpp >= 8 && mi.lxres >= 320) {
    for (i=15;i>=0;i--) {
      if (i==8)
	continue;
      pgNewWidget(PG_WIDGET_CANVAS,0,0);
      pgSetWidget(PGDEFAULT,
		  PG_WP_SIDE,PG_S_RIGHT,
		  0);
      /* 15-color EGA-like palette */
      makeColorSample(PGDEFAULT,
		    (i & 0x08) ?
		      (((i & 0x04) ? 0xFF0000 : 0) |
		       ((i & 0x02) ? 0x00FF00 : 0) |
		       ((i & 0x01) ? 0x0000FF : 0)) :
		      (((i & 0x04) ? 0x800000 : 0) |
		       ((i & 0x02) ? 0x008000 : 0) |
		       ((i & 0x01) ? 0x000080 : 0)));
    }
  }
  /* Draw a simple grayscale palette */
  else {
    pgNewWidget(PG_WIDGET_CANVAS,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0);
    makeColorSample(PGDEFAULT,0xFFFFFF);
    pgNewWidget(PG_WIDGET_CANVAS,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0);
    makeColorSample(PGDEFAULT,0xAAAAAA);
    pgNewWidget(PG_WIDGET_CANVAS,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0);
    makeColorSample(PGDEFAULT,0x000000);
  }

  /* Add some buttons to the menu */
  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wMenuTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Load..."),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtnLoad,NULL);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Save"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtnSave,NULL);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Save As..."),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtnSaveAs,NULL);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Clear..."),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtnClear,NULL);
 
  setCurrentColor(0x000000);   /* Default foreground and background */
  clearBackground(0xFFFFFF);
  updateAppTitle();

  pgEventLoop();
  return 0;
}

/******************************************** Event Handlers */

int evtColorSample(struct pgEvent *evt) {
  setCurrentColor((pgcolor) evt->extra);
  return 0;
}

int evtBtnOther(struct pgEvent *evt) {
  pgcolor c = currentColor;
  if (pgColorPicker(&c, "Pick a pen color"))
    setCurrentColor(c);
  return 0;
}

int evtCanvas(struct pgEvent *evt) {
  static pgu old_x = 0, old_y = 0;
  static int down;

  switch (evt->type) {
    
  case PG_WE_PNTR_DOWN:
    down = 1;
    old_x = evt->e.pntr.x;
    old_y = evt->e.pntr.y;
    break;

  case PG_WE_PNTR_UP:
  case PG_WE_PNTR_RELEASE:
    down = 0;
  case PG_WE_PNTR_MOVE:
    if (!down)
      break;

    /* Draw on the screen and on the bitmap */
    drawPenLine(gcMain,old_x,old_y,evt->e.pntr.x,evt->e.pntr.y);
    drawPenLine(gcBitmap,old_x,old_y,evt->e.pntr.x,evt->e.pntr.y);

    old_x = evt->e.pntr.x;
    old_y = evt->e.pntr.y;
    break;

  }
  return 0;
}

int evtBtnClear(struct pgEvent *evt) {
  if (pgMessageDialog(APP_NAME,"Clear the screen using the current color?",
		      PG_MSGBTN_YES | PG_MSGBTN_NO) == PG_MSGBTN_YES)
    clearBackground(currentColor);
  return 0;
}

/* If they try to close with out saving, ask first */
int evtClose(struct pgEvent *evt) {
  if (!needs_save)
    return 0;
  
  switch (pgMessageDialog(APP_NAME,"Save before closing?",
			  PG_MSGBTN_YES | PG_MSGBTN_NO | PG_MSGBTN_CANCEL)) {
    
  case PG_MSGBTN_NO:       /* Go ahead and close */
    return 0;

  case PG_MSGBTN_CANCEL:   /* Stop the close event */
    return 1;

  case PG_MSGBTN_YES:      /* Save */
    evtBtnSave(evt);
    return 0;
  }
  return 1;                /* Shouldn't get here */
}

int evtBtnLoad(struct pgEvent *evt) {
  const char *newfile;
  pghandle bLoaded;
  int loaded_w,loaded_h,scribble_w,scribble_h;

  /* Select a file with the file picker */
  newfile = pgFilePicker(NULL,NULL,currentFile,PG_FILEOPEN,
			 "Load a " APP_NAME " file");
  if (!newfile)
    return 0;

  /* It's the current file now */
  if (currentFile)
    free(currentFile);
  currentFile = strdup(newfile);

  /* Load it
   * FIXME: Trap errors here and report them to the user in a less
   * threatening way
   */
  bLoaded = pgNewBitmap(pgFromFile(currentFile));
  pgSizeBitmap(&loaded_w,&loaded_h,bLoaded);
  pgSizeBitmap(&scribble_w,&scribble_h,bScribbleBitmap);

  /* Blit it */
  pgBitmap(gcBitmap,0,0,loaded_w,loaded_h,bLoaded);

  /* If the bitmap's size doesn't match the current scribble document,
   * warn the user
   */
  if (loaded_w!=scribble_w || loaded_h!=scribble_h)
    pgMessageDialog("Warning - " APP_NAME,
		    "The size of the picture you are loading is\n"
		    "different than the size of this scribble pad.\n"
		    "\n"
		    "If you save this file, it will be saved in this\n"
		    "new format.",0);

  /* Clean up */
  pgDelete(bLoaded);

  needs_save = 0;
  updateAppTitle();
  return 0;
}

int evtBtnSave(struct pgEvent *evt) {
  /* Make sure we have a name */
  if (!currentFile)
    return evtBtnSaveAs(evt);

  /* FIXME: Save not implemented */
  pgMessageDialog(APP_NAME,"Save is not yet implemented",0);

  needs_save = 0;
  updateAppTitle();
  return 0;
}

int evtBtnSaveAs(struct pgEvent *evt) {
  const char *newfile;

  /* Select a file with the file picker */
  newfile = pgFilePicker(NULL,NULL,currentFile,PG_FILESAVE,
			 "SAVE a " APP_NAME " file");
  if (!newfile)
    return 0;

  /* It's the current file now */
  if (currentFile)
    free(currentFile);
  currentFile = strdup(newfile);
  
  /* Normal save */
  return evtBtnSave(evt);
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
  /* Update the app's title to reflect the current file */

  /* Make a 'nice' filename- cut off the path if there is one, and if
   * it hasn't been saved call it "Untitled"
   */
  const char *filename;
  if (currentFile) {
    filename = strrchr(currentFile,'/');
    if (filename)
      filename++;
    else
      filename = currentFile;
  }
  else
    filename = "Untitled";

  pgReplaceTextFmt(wApp,"%s - %s%s",filename,APP_NAME,
		   needs_save ? " (modified)" : "");
  pgSubUpdate(wApp);
}


/* The End */
