/* $Id: driverinfo.c,v 1.12 2001/01/16 02:07:19 micahjd Exp $
 *
 * driverinfo.c - has a static array with information about
 *                installed drivers
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

#include <pgserver/video.h>
#include <pgserver/input.h>

/* Video 
 * Order does matter here- if no driver is specified, the one listed 
 * first here will be tried first, trying the next if there is an error,
 * and so on...
 */
struct vidinfo videodrivers[] = {

#ifdef DRIVER_SDLFB
  {"sdlfb",&sdlfb_regfunc},
#endif
   
#ifdef DRIVER_SDL
  {"sdl",&sdl_regfunc},
#endif
   
#ifdef DRIVER_SVGAFB      
  {"svgafb",&svgafb_regfunc},
#endif

#ifdef DRIVER_SVGAGL
  {"svgagl",&svgagl_regfunc},
#endif   

#ifdef DRIVER_EZ328_CHIPSLICE
#  ifdef DRIVER_EZ328_CHIPSLICE_V0_2_CITIZEN_G3243H
     {"ez328_chipslice_vd2_citizen_G3243H",&chipslice_video_regfunc},
#  endif
#endif

#ifdef DRIVER_NCURSES
  {"ncurses",&ncurses_regfunc},
#endif   

#ifdef DRIVER_NULL
  {"null",&null_regfunc},
#endif DRIVER_NULL
   
  /* End */ {NULL,NULL}
};

/* Input
 * Usually this will be autoloaded by the video driver.  If no driver is
 * specified, no input. Order does not matter
 */
struct inputinfo inputdrivers[] = {

#ifdef DRIVER_SDLINPUT
  {"sdlinput",&sdlinput_regfunc},
#endif
   
#ifdef DRIVER_SVGAINPUT
  {"svgainput",&svgainput_regfunc},
#endif

#ifdef DRIVER_NCURSESINPUT
  {"ncursesinput",&ncursesinput_regfunc},
#endif

  /* End */ {NULL,NULL}
};

/* The End */
