/* $Id: mode.h,v 1.6 2000/06/01 23:11:42 micahjd Exp $
 *
 * Definitions for 640x480x32 mode
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
typedef Uint32 devcolort;
typedef Uint32 * devbmpt;

#define SDLFLAG SDL_FULLSCREEN

/* Geometry */
#define HWR_WIDTH   640
#define HWR_HEIGHT  480
#define HWR_BPP     32
#define HWR_BPPMASK 0xFFFFFFFF
#define HWR_PIXELW  4
#define HWR_LINEW   (HWR_PIXELW*HWR_WIDTH)
#define TITLE       HWR
#define SDL_BPP     32
#define DIM_ALGO(x) (((x)>>1) & HWR_BPPMASK)

/* Macro to make a color from RGB values 
 * Values between 0 and 255
 */
#define mkcolor(r,g,b) ((devcolort) (((r)<<16) | ((g)<<8) | (b)))
/* Reverses of mkcolor */
#define getred(c) (((c)>>16)&0xFF)
#define getgreen(c) (((c)>>8)&0xFF)
#define getblue(c) ((c)&0xFF)
/* Same thing, but for a grayscale v between 0 and m */
#define mkgray(v,m) mkcolor(v*255/m,v*255/m,v*255/m)

/* The End */
