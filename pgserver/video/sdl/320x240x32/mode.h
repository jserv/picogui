/* $Id: mode.h,v 1.7 2000/04/29 21:57:45 micahjd Exp $
 *
 * Definitions for 320x240x32 mode. Not very practical for real applications,
 * but it's handy to use a less-than-full-screen mode for testing.
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

#define SDLFLAG 0

/* Geometry */
#define HWR_WIDTH   320
#define HWR_HEIGHT  240
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


/* Generic colors */
#define black   0x00000000
#define white   0x00FFFFFF
#define gray    0x00808080
#define dkgray  0x00404040
#define ltgray  0x00B0B0B0

/* default theme colors */
#define dtc_toolbar 0x00C0D8E8
#define dtc_btn     0x008080FF

/* indicator */
#define ind_b1     0x00B0B0B0
#define ind_b2     0x00000000
#define ind_b3     0x008080FF
#define ind_midon  0x000000FF
#define ind_midoff 0x00FFFFFF

/* button */
#define btn_b1     0x00000000
#define btn_b2     0x00585858
#define btn_on     0x009090B0
#define btn_off    0x00B0B0B0
#define btn_over   0x00FFFFB0

/* Hardware-customized Widget Geometry */
#define HWG_BUTTON  25
#define HWG_MARGIN  3
#define HWG_SCROLL  8
#define HWG_SPACER  2
#define HWG_POPUPW  240
#define HWG_POPUPH  130

/* The End */
