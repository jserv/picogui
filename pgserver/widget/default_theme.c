/* $Id: default_theme.c,v 1.11 2000/10/10 00:33:37 micahjd Exp $
 *
 * default_theme.h - Theme table initialized with the default theme
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

#include <pgserver/theme.h>
#include <pgserver/video.h>

/* Little macros for a theme element.
 * This format is more limiting than the actual theme definition format,
 * but the default theme needs to be simple enough to work on any hardware.
 */
#define EL_NULL {0,PG_ELEM_NULL}
#define EL_FLAT(w,c) {w,PG_ELEM_FLAT,{{c},{c},{c}}}
#define EL_ACTIVE(c1,c2,c3) {0,PG_ELEM_FLAT,{{c1},{c2},{c3}}}

/* This is initialized to the default theme */
struct element default_theme[PG_E_NUM] = {

  /* 00 button.border */
  EL_FLAT(1,mkcolor(0,0,0)),

  /* 01 button.fill */
  EL_ACTIVE(mkcolor(192,224,255),mkcolor(192,192,255),mkcolor(128,128,128)),
  
  /* 02 button.overlay */
  EL_NULL,

  /* 03 toolbar.border */
  EL_FLAT(-1,mkcolor(0,0,0)),  

  /* 04 toolbar.fill */
  EL_FLAT(0,mkcolor(192,192,192)),

  /* 05 scrollbar.border */ 
  EL_FLAT(-1,mkcolor(0,0,0)),  

  /* 06 scrollbar.fill */
  EL_FLAT(0,mkcolor(128,128,128)),

  /* 07 scrollind.border */
  EL_FLAT(1,mkcolor(0,0,0)),  

  /* 08 scrollind.fill */
  EL_ACTIVE(mkcolor(255,255,192),mkcolor(192,224,255),mkcolor(192,192,255)),

  /* 09 scrollind.overlay */
  EL_NULL,

  /* 10 indicator.border */
  EL_FLAT(1,mkcolor(0,0,0)),  

  /* 11 indicator.fill */
  EL_FLAT(0,mkcolor(64,64,128)),  

  /* 12 indicator.overlay */
  EL_FLAT(0,mkcolor(128,128,255)),  

  /* 13 panel.border */
  EL_NULL,

  /* 14 panel.fill */
  EL_FLAT(0,mkcolor(255,255,255)),  

  /* 15 panelbar.border */
  EL_FLAT(1,mkcolor(64,64,64)),  

  /* 16 panelbar.fill */
  EL_ACTIVE(mkcolor(255,255,192),mkcolor(192,224,255),mkcolor(192,192,255)),

  /* 17 popup.border */
  EL_FLAT(1,mkcolor(0,0,0)),  

  /* 18 popup.fill */
  EL_FLAT(0,mkcolor(255,255,224)),  

};

/* The End */










