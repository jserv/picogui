/* $Id: appmgr.h,v 1.3 2000/10/10 00:33:37 micahjd Exp $
 *
 * appmgr.h - All the window-manager-ish functionality, except we don't
 * do windows (X windows, that is?)
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

#ifndef __H_APPMGR
#define __H_APPMGR

#include <picogui/constants.h>

/* Parameters defining an application */
struct app_info {
  /* These should be provided by the client */
  handle name;
  int type;
  int minw,maxw,minh,maxh;     /* 0 for no restrictions */
  int sidemask;                /* Mask of allowed sides */

  /* Filled initially with suggested values, later replaced by actual
     values. */
  int side;
  int w,h;   /* w is used if side is a vertical split, h if it is a
		horizontal split */

  /* This should be managed by the request system. */
  int owner;

  /* Root widget handle (Filled in on app registration) */
  handle rootw; 

  /* Yep, it's a linked list */
  struct app_info *next;
};

/* Global objects */
extern handle defaultfont;
extern struct app_info *applist;

/* Init & Free */
g_error appmgr_init(void);
void appmgr_free(void);

/* Pass it a bitmap handle, or NULL to restore default background */
g_error appmgr_setbg(int owner,handle bitmap);

/* Register a new application. 
   Fill out the app_info structure, then call this.
   The app_info structure can be on the stack - a dynamically allocated
   copy is stored.
*/
g_error appmgr_register(struct app_info *i);

/* Unregisters applications owned by a given connection */
void appmgr_unregowner(int owner);

#endif /* __H_APPMGR */
/* The End */
