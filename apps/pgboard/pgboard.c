/* $Id: pgboard.c,v 1.8 2001/10/23 17:25:05 cgrigis Exp $
 *
 * pgboard.c - Onscreen keyboard for PicoGUI on handheld devices. Loads
 *             a keyboard definition file containing one or more 'patterns'
 *             with key layouts.
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
 * 
 * 
 * 
 */

#include <stdio.h>
#include <picogui.h>
#include <picogui/pgboard.h>
#include "kbfile.h"

FILE *fpat;
struct mem_pattern mpat;
pghandle wCanvas, wApp;
struct key_entry *keydown = NULL;
int current_pat;

int initKbdCanvas ();
void disableKbdCanvas ();

/* Small utility function to XOR a key's rectangle */
void xorKey(struct key_entry *k) {
  pgcontext gc;
  if (!k)
    return;
  gc = pgNewCanvasContext(wCanvas,PGFX_IMMEDIATE);
  pgSetLgop(gc,PG_LGOP_XOR);
  pgSetColor(gc,0xFFFFFF);
  pgRect(gc,k->x,k->y,k->w,k->h);
  pgContextUpdate(gc);
  pgDeleteContext(gc);
}

/*
 * Handler for messages sent to the keyboard
 */
int evtMessage (struct pgEvent * evt)
{
  /* Received command */
  struct keyboard_command * cmd;

  /* Keyboard's new size */
  int newSize = -1;

  /* Internal status reprenting the stats of the keyboard (enabled/disabled) */
  static int status = 0;
      
  switch (evt->type)
    {
    case PG_WE_APPMSG:
      cmd = (struct keyboard_command *) evt->e.data.pointer;

      switch (cmd->type)
	{
	case PG_KEYBOARD_SHOW:
	  newSize = mpat.app_size;
	  break;

	case PG_KEYBOARD_HIDE:
	  newSize = 0;
	  break;

	case PG_KEYBOARD_TOGGLE:
	  newSize = mpat.app_size - pgGetWidget (wApp, PG_WP_SIZE);
	  break;

	case PG_KEYBOARD_ENABLE:
	  initKbdCanvas ();
	  status = 0;
	  break;

	case PG_KEYBOARD_DISABLE:
	  disableKbdCanvas ();
	  status = 1;
	  break;

	case PG_KEYBOARD_TOGGLE_DISPLAY:
	  if (status)
	    {
	      initKbdCanvas ();
	    }
	  else
	    {
	      disableKbdCanvas ();
	    }
	  status = !status;
	  break;

	default:
	  printf ("Unknown command: %d\n", cmd->type);
	  break;
	}

      if (newSize >= 0)
	{
	  pgSetWidget(wApp,
		      PG_WP_SIZE, newSize,
		      0);
	}
      
      break;
     
    default:
      /* Ignore */
      break;
    }

  return 1;
}

int evtMouse(struct pgEvent *evt) {
  struct key_entry *k, *clickkey = NULL;
  short n;

  /* Ignore all but left mouse button */
  if (evt->e.pntr.chbtn != 1 && evt->type != PG_WE_PNTR_RELEASE)
    return 0;

  /* Figure out what (if anything) was clicked */
  for (k=mpat.keys,n=mpat.num_keys;n;n--,k++) {
    if (evt->e.pntr.x < k->x) continue;
    if (evt->e.pntr.x > (k->x+k->w-1)) continue;
    if (evt->e.pntr.y < k->y) continue;
    if (evt->e.pntr.y > (k->y+k->h-1)) continue;
   
    clickkey = k;
    break;
  }

  /* If we got this far, it was clicked */
  if (evt->type == PG_WE_PNTR_DOWN) {
    keydown = clickkey;
     
    if (clickkey) {
      if (clickkey->key)
	pgSendKeyInput(PG_TRIGGER_CHAR,clickkey->key,clickkey->mods);
      if (clickkey->pgkey)
	pgSendKeyInput(PG_TRIGGER_KEYDOWN,clickkey->pgkey,clickkey->mods);
    }
  }
  else {
    if (keydown!=clickkey) {
      xorKey(keydown);
      keydown = NULL;
      return 0;
    }
    else
      if (clickkey) {
	if (clickkey->pgkey)
	  pgSendKeyInput(PG_TRIGGER_KEYUP,clickkey->pgkey,clickkey->mods);
	if (clickkey->pattern && clickkey->pattern-1 != current_pat) {
	  pgLeaveContext();
	  pgEnterContext();
	  kb_loadpattern(fpat,&mpat,
			 current_pat = clickkey->pattern-1,wCanvas);
	  pgWriteCmd(wCanvas,PGCANVAS_REDRAW,0);
	  pgSubUpdate(wCanvas);
	  keydown = NULL;
	  return 0;
	}
      }
     
    keydown = NULL;
  }
   
  /* Flash the clicked key with an XOR'ed rectangle */
  xorKey(clickkey);

  return 0;
}

/*
 * Initialize the keyboard canvas
 */
int initKbdCanvas ()
{
  if (mpat.keys == 0)
    {
      rewind (fpat);
      pgEnterContext ();
      if (kb_loadpattern (fpat, &mpat, current_pat = 0, wCanvas)) {
	pgMessageDialog("Virtual Keyboard", "Error loading keyboard pattern", 0);
	return 1;
      }
    }
  pgWriteCmd (wCanvas, PGCANVAS_REDRAW, 0);
  pgSubUpdate (wCanvas);
  
  /* Set up an event handler for the keyboard */
  pgBind (wCanvas, PG_WE_PNTR_DOWN, &evtMouse, NULL);
  pgBind (wCanvas, PG_WE_PNTR_RELEASE, &evtMouse, NULL);
  pgBind (wCanvas, PG_WE_PNTR_UP, &evtMouse, NULL);

  return 0;
}

/*
 * Disable the keyboard canvas by clearing it
 */
void disableKbdCanvas ()
{
  pgcontext gc;

  gc = pgNewCanvasContext (wCanvas, PGFX_IMMEDIATE);
  pgSetColor (gc, 0xFFFFFF);
  pgRect (gc, 0, 0, 100, pgGetWidget (wApp, PG_WP_SIZE));
  pgContextUpdate (gc);
  pgDeleteContext (gc);

  /* Clear event handlers for the keyboard */
  pgBind (wCanvas, PG_WE_PNTR_DOWN, NULL, NULL);
  pgBind (wCanvas, PG_WE_PNTR_RELEASE, NULL, NULL);
  pgBind (wCanvas, PG_WE_PNTR_UP, NULL, NULL);
}

int main(int argc,char **argv) {
  /* Make a 'toolbar' app */
  pgInit(argc,argv);
  wApp = pgRegisterApp(PG_APP_TOOLBAR,"Keyboard",0);

  wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);

  if (!argv[1] || argv[2]) {
    printf("usage:\n  %s <keyboard file>\n",argv[0]);
    return 1;
  }

  /* Load a pattern */
  memset(&mpat,0,sizeof(mpat));
  fpat = fopen(argv[1],"r");
  if (!fpat) {
    pgMessageDialog(*argv,"Error loading keyboard file",0);
    return 1;
  }
  if (kb_validate(fpat,&mpat)) {
    pgMessageDialog(*argv,"Invalid keyboard file",0);
    return 1;
  }

  /* Resize app widget */
  pgSetWidget(wApp,
              PG_WP_NAME,pgNewString(PG_KEYBOARD_APPNAME),
	      PG_WP_SIDE,mpat.app_side,
	      PG_WP_SIZE,mpat.app_size,
	      PG_WP_SIZEMODE,mpat.app_sizemode,
	      PG_WP_TRANSPARENT,1,
	      0);

  initKbdCanvas ();

  /* Set up an event handler for the received messages */
  pgBind (wApp, PG_WE_APPMSG, &evtMessage, NULL);

  pgEventLoop();
  return 0;
}


