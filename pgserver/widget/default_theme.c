/* $Id: default_theme.c,v 1.1 2000/06/01 23:11:42 micahjd Exp $
 *
 * default_theme.h - Theme table initialized with the default theme
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <theme.h>
#include <video.h>

/* Little macros for a theme element.
 * This format is more limiting than the actual theme definition format,
 * but the default theme needs to be simple enough to work on any hardware.
 */
#define EL_NULL {0,0,0,0,ELEM_NULL}
#define EL_FLAT(w,c) {w,w,-2*w,-2*w,ELEM_FLAT,{{c}}}
#define EL_ACTIVE(c1,c2,c3) {0,0,0,0,ELEM_FLAT,{{c1},{c2},{c3}}}

/* This is initialized to the default theme */
struct element current_theme[E_NUM] = {

  /* 00 button.border */
  EL_FLAT(1,mkcolor(0,0,0)),

  /* 01 button.fill */
  EL_ACTIVE(mkcolor(255,255,255),mkcolor(200,200,255),mkcolor(127,127,255)),
  
  /* 02 button.overlay */
  EL_NULL,

  /* 03 toolbar.border */
  EL_FLAT(1,mkcolor(0,0,0)),  

  /* 04 toolbar.fill */
  EL_FLAT(0,mkcolor(220,255,220)),

  /* 05 scrollbar.border */ 
  EL_FLAT(1,mkcolor(0,0,0)),  

  /* 06 scrollbar.fill */
  EL_FLAT(0,mkcolor(220,220,255)),

  /* 07 scrollind.border */
  EL_FLAT(1,mkcolor(0,0,0)),  

  /* 08 scrollind.fill */
  EL_FLAT(0,mkcolor(220,255,220)),  

  /* 09 scrollind.overlay */
  EL_NULL,

  /* 10 indicator.border */
  EL_FLAT(1,mkcolor(0,0,0)),  

  /* 11 indicator.fill */
  EL_FLAT(0,mkcolor(220,255,220)),  

  /* 12 indicator.overlay */
  EL_FLAT(0,mkcolor(255,255,0)),  

  /* 13 panel.border */
  EL_NULL,

  /* 14 panel.fill */
  EL_FLAT(0,mkcolor(255,255,255)),  

  /* 15 panelbar.border */
  EL_FLAT(1,mkcolor(127,127,127)),  

  /* 16 panelbar.fill */
  EL_FLAT(0,mkcolor(192,192,192)),  

};

/* The End */




