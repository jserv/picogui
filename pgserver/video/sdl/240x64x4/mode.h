/* $Id: mode.h,v 1.4 2000/04/24 02:38:36 micahjd Exp $
 *
 * Definitions for kiwi-compatible 240x64x4 mode
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

#include <SDL.h>

/* Define the data types */
typedef Uint8 devcolort;
typedef Uint8 * devbmpt;

#define SDLFLAG 0

/* Geometry */
#define HWR_WIDTH   240  /* Emulate 240x64, 16 grayscale */
#define HWR_HEIGHT  64
#define HWR_BPP     4
#define HWR_BPPMASK 0x0F
#define HWR_PIXELW  1
#define HWR_LINEW   (HWR_PIXELW*HWR_WIDTH)
#define TITLE       HWR
#define SDL_BPP     8
#define DIM_ALGO(x) (((x)>>1) & HWR_BPPMASK)

/* Macro to make a color from RGB values (grayscale weighted average) 
 * Values between 0 and 255
 */
#define mkcolor(r,g,b) ((devcolort) (((((r)*3+(g)*6+(b))/10)>>4) & 0x0F))
/* Same thing, but for a grayscale v between 0 and m */
#define mkgray(v,m) ((devcolort) ((v) * white / (m)))


/* Generic colors */
#define black   0
#define white   15
#define gray    7
#define dkgray  3
#define ltgray  11

/* panel */
#define panelmid  ltgray
#define paneledge black

/* title bar */
#define titlebg   black

/* indicator */
#define ind_b1     white
#define ind_b2     black
#define ind_b3     ltgray
#define ind_midon  gray
#define ind_midoff white

/* button */
#define btn_b1     black
#define btn_b2     dkgray
#define btn_on     dkgray
#define btn_off    ltgray
#define btn_over   white

/* Hardware-customized Widget Geometry */
#define HWG_BUTTON  16
#define HWG_MARGIN  2
#define HWG_SCROLL  5
#define HWG_SPACER  1
#define HWG_POPUPW  170
#define HWG_POPUPH  58  /* This is the minimum height to fit 3 buttons */

/* The End */
