/* $Id: pgboard.c,v 1.18 2001/11/07 17:38:50 cgrigis Exp $
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

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef POCKETBEE
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif /* POCKETBEE */
#include <netinet/in.h>
#include <picogui.h>
#include "pgboard.h"
#include "kbfile.h"

/* as some systems don't have htons primitives (as uClinux) */
#ifdef _NEED_NTOHS_FRIENDS_
    #include "ntohs_fr.h"
#else
    #include <netinet/in.h>
#endif

FILE *fpat;
struct mem_pattern * mpat;
pghandle wCanvas, wDisabled, wApp;
struct key_entry *keydown = NULL;

/* Flag representing the status of the keyboard (enabled/disabled) */
static int enable_status = 1;
/* Current pattern */
static unsigned short current_patnum;

/* Structure to hold the current keyboard context */
struct keyboard_context
{
  long appSize;
  int enable_status;
  unsigned short current_patnum;

  /* Linked list pointer */
  struct keyboard_context * next;
};

/* Top of the keyboard context stack */
static struct keyboard_context * context_top = NULL;

/* Prototype declarations */
void enableKbdCanvas ();
void disableKbdCanvas ();
void selectPattern (unsigned short);

/*
 * Push the current keyboard context 
 */
void pushKeyboardContext ()
{
  struct keyboard_context * kbc;

  /* Allocate new context */
  kbc = (struct keyboard_context *) malloc (sizeof (struct keyboard_context));
  if (kbc == NULL)
    {
      pgMessageDialog ("Virtual Keyboard", "Error allocating memory for context", 0);
      return;
    }

  /* Fill in context data */
  kbc->appSize = pgGetWidget (wApp, PG_WP_SIZE);
  kbc->enable_status = enable_status;
  kbc->current_patnum = current_patnum;
  kbc->next = context_top;

  /* Update top of context stack */
  context_top = kbc;
}

/*
 * Pop the last pushed keyboard context
 */
void popKeyboardContext ()
{
  struct keyboard_context * kbc = context_top;

  if (context_top == NULL)
    {
      /* No context to pop -> ignore */
      printf ("popKeyboardContext() on empty context stack --> ignored\n");
      return;
    }

  /* Retrieve context data */
  pgSetWidget(wApp,
	      PG_WP_SIZE, kbc->appSize,
	      0);
  enable_status = kbc->enable_status;
  enable_status ? enableKbdCanvas () : disableKbdCanvas ();
  selectPattern (kbc->current_patnum);

  /* Update top of context stack */
  context_top = kbc->next;

  /* Free context space */
  free (kbc);
}

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

  switch (evt->type)
    {
    case PG_WE_APPMSG:
      cmd = (struct keyboard_command *) evt->e.data.pointer;

      /* Command structure is in network byte order */
      cmd->type = ntohs(cmd->type);

      switch (cmd->type)
	{
	case PG_KEYBOARD_SHOW:
	  newSize = mpat->app_size;
	  break;

	case PG_KEYBOARD_HIDE:
	  newSize = 0;
	  break;

	case PG_KEYBOARD_TOGGLE:
	  newSize = mpat->app_size - pgGetWidget (wApp, PG_WP_SIZE);
	  break;

	case PG_KEYBOARD_ENABLE:
	  enableKbdCanvas ();
	  enable_status = 1;
	  break;

	case PG_KEYBOARD_DISABLE:
	  disableKbdCanvas ();
	  enable_status = 0;
	  break;

	case PG_KEYBOARD_TOGGLE_DISPLAY:
	  enable_status = !enable_status;
	  enable_status ? enableKbdCanvas () : disableKbdCanvas ();
	  break;

	case PG_KEYBOARD_SELECT_PATTERN:
	  selectPattern (ntohs (cmd->data.pattern));
	  break;

	case PG_KEYBOARD_PUSH_CONTEXT:
	  pushKeyboardContext ();
	  break;

	case PG_KEYBOARD_POP_CONTEXT:
	  popKeyboardContext ();
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

/*
 * Select a given key pattern
 * pattern : key pattern identifier
 */
void selectPattern (unsigned short pattern)
{
  if (pattern == current_patnum)
    {
      /* Shortcut if we are selecting the current pattern */
      return;
    }

  kb_selectpattern (pattern - 1, wCanvas);
  current_patnum = pattern;
}

int evtMouse(struct pgEvent *evt) {
  struct key_entry *clickkey = NULL;

  /* Ignore all but left mouse button */
  if (evt->e.pntr.chbtn != 1 && evt->type != PG_WE_PNTR_RELEASE)
    return 0;

  /* Figure out what (if anything) was clicked */
  clickkey = find_clicked_key (evt->e.pntr.x, evt->e.pntr.y);

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
	if (clickkey->pattern) {
	  selectPattern (clickkey->pattern);
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
  selectPattern (PG_KBPATTERN_NORMAL);
  
  /* Set up an event handler for the keyboard */
  pgBind (wCanvas, PG_WE_PNTR_DOWN, &evtMouse, NULL);
  pgBind (wCanvas, PG_WE_PNTR_RELEASE, &evtMouse, NULL);
  pgBind (wCanvas, PG_WE_PNTR_UP, &evtMouse, NULL);

  return 0;
}

/*
 * Enable the keyboard canvas
 */
void enableKbdCanvas ()
{
  pgSetWidget (wCanvas,
	       PG_WP_SIDE, PG_S_ALL,
	       0);
}

/*
 * Disable the keyboard canvas
 */
void disableKbdCanvas ()
{
  pgSetWidget (wCanvas,
	       PG_WP_SIZE, 0,
	       PG_WP_SIDE, PG_S_TOP,
	       0);
}

/*
 * Draw the contents of the disabled keyboard
 * This could be filled with an image, a logo, etc.
 */
void drawDisabledKeyboard ()
{
  pgcontext gc = pgNewCanvasContext (wDisabled, PGFX_PERSISTENT);

  pgSetMapping (gc, 0, 0, 100, 100, PG_MAP_SCALE);
  pgSetColor (gc, 0xFFFFFF);
  pgRect (gc, 0, 0, 100, 100);
  /* ... */

  pgContextUpdate (gc);
  pgDeleteContext (gc);
}

#ifdef POCKETBEE
/*
 * Release the lock preventing multiple instances.
 */
void release_lock ()
{
  /* Delete lock file */
  unlink (PG_KEYBOARD_LOCKFILE);
}

/*
 * Handler for TERM signal.
 */
void sig_handler (int sig)
{
  release_lock ();
  _exit (1);
}
#endif /* POCKETBEE */

int main(int argc,char **argv) {
  /* File data */
  unsigned char * file_data;

#ifdef POCKETBEE
  /* Register the handler for SIGTERM */
  signal (SIGTERM, sig_handler);
  /* Register exit function */
  atexit (release_lock);

  /* Try to obtain the file lock */
  if (open (PG_KEYBOARD_LOCKFILE, O_CREAT | O_EXCL) == -1 && errno == EEXIST)
    {
      /* File already locked -> another instance is running -> exit */
      _exit (1);
    }
#endif /* POCKETBEE */

  /* Make a 'toolbar' app */
  pgInit(argc,argv);
  wApp = pgRegisterApp(PG_APP_TOOLBAR,"Keyboard",0);

  wCanvas = pgNewWidget(PG_WIDGET_CANVAS, PG_DERIVE_INSIDE, wApp);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_ALL,
	       0);
  wDisabled = pgNewWidget (PG_WIDGET_CANVAS, PG_DERIVE_AFTER, wCanvas);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_ALL,
	       0);
  drawDisabledKeyboard ();

  if (!argv[1] || argv[2]) {
    printf("usage:\n  %s <keyboard file>\n",argv[0]);
    return 1;
  }

  /* Load patterns */
  fpat = fopen(argv[1],"r");
  if (!fpat) {
    pgMessageDialog(*argv,"Error loading keyboard file",0);
    return 1;
  }
  file_data = kb_validate (fpat, &mpat);
  fclose (fpat);
  if (file_data == NULL) {
    pgMessageDialog(*argv,"Invalid keyboard file [kb_validate()]",0);
    return 1;
  }
  if (kb_loadpatterns(file_data)) {
    free (file_data);
    pgMessageDialog(*argv,"Invalid keyboard file [kb_loadpatterns()]",0);
    return 1;
  }
  free (file_data);

  /* Resize app widget */
  pgSetWidget(wApp,
              PG_WP_NAME,pgNewString(PG_KEYBOARD_APPNAME),
	      PG_WP_SIDE,mpat->app_side,
	      PG_WP_SIZE,mpat->app_size,
	      PG_WP_SIZEMODE,mpat->app_sizemode,
	      PG_WP_TRANSPARENT,1,
	      0);

  initKbdCanvas ();

  /* Set up an event handler for the received messages */
  pgBind (wApp, PG_WE_APPMSG, &evtMessage, NULL);

#ifdef POCKETBEE
  /* Signal the parent of a proper start */
  kill (getppid (), SIGUSR1);
#endif /* POCKETBEE */

  pgEventLoop();
  return 0;
}


