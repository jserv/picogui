/* $Id: pgstring.h,v 1.1 2002/07/05 05:48:41 micahjd Exp $
 *
 * pgstring.h - String data type to handle various encodings, and managing string length
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef __PGSTRING_H
#define __PGSTRING_H

/* String encodings */
#define PG_STRINGTYPE_


struct pgstring {
  u8 *buffer;
  int buffer_bytes;           /* Total buffer length in bytes */
  int num_chars;              /* Number of characters used in the buffer */
  int flags;                  /* Encoding flags */
  int (*encoder)(const u8 **dest, int src);
}

pgstring_decode



#endif /* __PGSTRING_H */
/* The End */
