/* $Id: mode.h,v 1.2 2000/06/04 20:30:52 micahjd Exp $
 *
 * A basic 256 color 640x480 SVGA mode
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

#include <vga.h>
#include <vgagl.h>

/* Define the data types */
typedef unsigned char devcolort;
typedef unsigned char * devbmpt;

#define VGA_MODE G640x480x256

/* Geometry */
#define HWR_WIDTH   640
#define HWR_HEIGHT  480
#define HWR_BPP     8
#define HWR_PIXELW  1
#define HWR_BPPMASK 0xFF

/* Mapping between RGB and a 2-3-3 palette */

#define mkcolor(r,g,b) ((devcolort)(((b)>>5)|(((g)>>2)&0x38)|((r)&0xC0)))
#define mkgray(v,m) mkcolor(v*255/m,v*255/m,v*255/m)
#define getred(c) ((c)&0xC0)
#define getgreen(c) (((c)<<2)&0xE0)
#define getblue(c) (((c)<<5)&0xE0)

/* The End */
