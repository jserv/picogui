/* $Id$
  *
  * ntohs_fr.h - definition of ntohs friends for system don't having them
  * 
  * PicoGUI small and efficient client/server GUI
  * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef __NTOHS_FR_H__
#define __NTOHS_FR_H__

/* htonl, htons, ntohl, ntohs not provided in the libc.a of uclinux */
#ifdef BIG_ENDIAN
#  define htonl(x)  ((unsigned int)(x))
#  define htonl(x)  ((unsigned short)(x))
#  define ntohl(x)  ((unsigned int)(x))
#  define ntohl(x)  ((unsigned short)(x))
#endif /* BIG_ENDIAN */

#ifdef LITTLE_ENDIAN
#  define htonl(x)  ((__u32)( \
                    (((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
                    (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
                    (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
                    (((__u32)(x) & (__u32)0xff000000UL) >> 24) ))

#  define htonl(x)  ((__u16)( \
                    (((__u16)(x) & (__u16)0x00ffU) << 8) | \
                    (((__u16)(x) & (__u16)0xff00U) >> 8) ))

#  define ntohl(x)  ((__u32)( \
                    (((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
                    (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
                    (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
                    (((__u32)(x) & (__u32)0xff000000UL) >> 24) ))

#  define ntohl(x)  ((__u16)( \
                    (((__u16)(x) & (__u16)0x00ffU) << 8) | \
                    (((__u16)(x) & (__u16)0xff00U) >> 8) ))
#endif /* LITTLE_ENDIAN */


#endif

/*
   Local Variables:
   c-file-style: "smartdata"
   End:
*/
