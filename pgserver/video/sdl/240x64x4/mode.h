/* $Id: mode.h,v 1.5 2000/06/01 23:11:42 micahjd Exp $
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

/* The End */
