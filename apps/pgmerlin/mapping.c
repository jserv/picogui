/**************************************************************************
 * mapping.c
 *
 * Copyright (c) 2000 Stefan Hellkvist, Ericsson Radio Systems AB 
 * Email: stefan@hellkvist.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *****************************************************************************/

#include "mapping.h"
#include "graphics.h"
#include <X11/Xlib.h>
#include <stdio.h>


struct binding
{
    unsigned int keycode; /* The keycode for that ascii character */
    unsigned int state;   /* The state to get that character, 
			     like shift etc */
};



struct binding _asciiToKeycode[] = {
#ifdef USE_ENGLISH
#include "keymap/english-kbd.map"
#else
#include "keymap/swedish-kbd.map"
#endif
};

    
unsigned int
asciiToKeycode( unsigned char c, unsigned int *state )
{
    *state = _asciiToKeycode[c].state;
    return _asciiToKeycode[c].keycode;
}


