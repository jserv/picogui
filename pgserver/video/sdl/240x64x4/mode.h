/*
 * Definitions for kiwi-compatible 240x64x4 mode
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

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
#define mkcolor(r,g,b) ((devcolort) 15-(((((r)*3+(g)*6+(b))/10)>>4) & 0x0F))
/* Same thing, but for a grayscale v between 0 and m */
#define mkgray(v,m) ((devcolort) black - ((v) * black / (m)))


/* Generic colors */
#define black   15
#define white   0
#define gray    7
#define dkgray  11
#define ltgray  3

/* Bevel colors */
#define bevmid  5
#define bevnw   12
#define bevse   9

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
