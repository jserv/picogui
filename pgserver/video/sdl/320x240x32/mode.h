/*
 * Definitions for 320x240x32 mode
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

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
/* Same thing, but for a grayscale v between 0 and m */
#define mkgray(v,m) mkcolor(v*255/m,v*255/m,v*255/m)


/* Generic colors */
#define black   0x00000000
#define white   0x00FFFFFF
#define gray    0x00808080
#define dkgray  0x00404040
#define ltgray  0x00B0B0B0

/* Bevel colors */
#define bevmid  0x007080A0
#define bevnw   0x008090A0
#define bevse   0x006070A0

/* panel */
#define panelmid  0x00C0D8E8
#define paneledge 0x00000000

/* title bar */
#define titlebg   0x00003000

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
#define HWG_BUTTON  20
#define HWG_MARGIN  3
#define HWG_SCROLL  8
#define HWG_SPACER  2
#define HWG_POPUPW  240
#define HWG_POPUPH  130

/* The End */
