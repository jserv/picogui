/* $Id: ez328_chipslice_lcd.h,v 1.1 2000/10/30 11:35:22 bauermeister Exp $
 *
 * ez328_chipslice_lcd.h
 *           Definitions of register and hardware-related stuff for
 *           the video controller embedded in the M68EZ328 AKA DragonBall.
 *
 *           Most parts are M68EZ328-standard, some few parts such as
 *           GPIO port mapping are specific to the plateform and/or the
 *           LCD-Panel.
 *
 * PicoGUI small and efficient client/server GUI
 * Project initiated by Micah Dowty <micahjd@users.sourceforge.net>
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
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch> :
 *   initial version
 * 
 */

#ifndef _H_EZ328_CHIPSLICE_LCD
#define _H_EZ328_CHIPSLICE_LCD

#include <asm/types.h>

#define uint32 __u32
#define uint16 __u16
#define uint8  __u8

#define LSSA    *(volatile uint8**)0xfffffa00
#define LVPW    *(volatile uint8* )0xfffffa05
#define LXMAX   *(volatile uint16*)0xfffffa08
#define LYMAX   *(volatile uint16*)0xfffffa0a
#define LCXP    *(volatile uint16*)0xfffffa18
#define LCYP    *(volatile uint16*)0xfffffa1a
#define LCWCH   *(volatile uint16*)0xfffffa1c
#define LBLKC   *(volatile uint8* )0xfffffa1f
#define LPICF   *(volatile uint8* )0xfffffa20
#define LPOLCF  *(volatile uint8* )0xfffffa21
#define LACD    *(volatile uint8* )0xfffffa23
#define LPXCD   *(volatile uint8* )0xfffffa25
#define LCKCON  *(volatile uint8* )0xfffffa27
#define LRRA    *(volatile uint8* )0xfffffa29
#define LPOSR   *(volatile uint8* )0xfffffa2d
#define LFRCM   *(volatile uint8* )0xfffffa31
#define LGPMR   *(volatile uint8* )0xfffffa33
#define PWM     *(volatile uint16*)0xfffffa36

#define PF      *(volatile uint8* )0xfffff429
#define PFRES   *(volatile uint8* )0xfffff42a
#define PFDIR   *(volatile uint8* )0xfffff428
#define PFSEL   *(volatile uint8* )0xfffff42b
#define PFMSK   0x01

#ifdef DRIVER_EZ328_CHIPSLICE_V0_2_CITIZEN_G3243H
  /* ChipSlice V0.2 has a Citizen G3243 LCD panel, and the on/off signal
  *  is mapped to bit0 of port F
  */
# define LCDENABLE  { PFRES|=PFMSK; PFDIR|=PFMSK; PFSEL|=PFMSK; PF|=PFMSK;}
# define LCDDISABLE { PFRES|=PFMSK; PFDIR|=PFMSK; PFSEL|=PFMSK; PF&=~PFMSK;}
#else
#endif

#endif /* _H_EZ328_CHIPSLICE_LCD */
