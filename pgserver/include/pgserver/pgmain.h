/* $Id: pgmain.h,v 1.11 2002/07/03 22:03:29 micahjd Exp $
 *
 * pgmain.h - just a few things related to the main loop
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef _H_PGMAIN
#define _H_PGMAIN

#include <pgserver/handle.h>

/* For storing theme files to load later */
struct themefilenode {
  char *name;
  handle h;
  struct themefilenode *next;
};

/* Variables indicating pgserver's status, they may be affected by signals
 */
extern volatile int mainloop_proceed;
extern volatile int in_init;
extern volatile int use_sessionmgmt;           /* Using session manager, exit after last client */
extern volatile int use_tpcal;                 /* Run tpcal before running the session manager */
extern volatile int sessionmgr_secondary;      /* Need to run session manager after tpcal */
extern volatile int sessionmgr_start;          /* Start the session manager at the next iteration */

/* Call request_quit to exit pgserver cleanly */
void request_quit(void);

/* This installs signal handlers in signals.c */
void signals_install(void);

/* Functions for initializing function pointer tables when they can't
 * be done at compile time (ucLinux, for example) */
#ifdef RUNTIME_FUNCPTR
void widgettab_init(void);
void drivertab_init(void);
void rqhtab_init(void);
#endif

/* This is called whenever video is reloaded at a higher color depth
 * to reload all themes passed on the command line */
g_error reload_initial_themes(void);

/* This loads a list of theme files into pgserver */
g_error load_themefile_list(struct themefilenode *list);

#endif /* _H_PGMAIN */
/* The End */
