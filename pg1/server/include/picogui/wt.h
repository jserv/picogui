/* $Id$
 * 
 * wt.h - Defines the Widget Template file format, used by the server and by
 *        programs that read and write Widget Template files
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#ifndef __WT_H
#define __WT_H

/* format version (file_ver) */
#define PGWT_FORMATVERSION  0x0001

/******** PicoGUI widget template file format ********/
/* All numbers in network byte order */

/* Every widget template starts with this header (20 bytes) */
struct pgwt_header {
  char magic[4];              /* = "PGwt" */ 

  u32 file_len;    /* Expected file length, or 0 to disable checking length and checksum */
  u32 file_crc32;  /* 32-bit checksum, using zlib's CRC-32 algorithm */

  u16 file_ver;    /* Format version */

  u16 num_global;   /* Requests to be loaded globally  */
  u16 num_instance; /* Number of per-instance requests */
  u16 num_handles;  /* Size of handle table to define  */
};

/*
 * This is followed by (num_global + num_instance) PicoGUI requests, with global requests
 * before instance-specific requests. 
 *
 * After each request is run, its result is dealt with according to the 'id' value of the
 * request:
 *
 *  - from 0 to (num-handles-1)
 *    Stick the result in this location in the handle table
 *
 *  - 0xFFFFFFFF
 *    Ignore it
 *
 *  - Other values
 *    Reserved
 *
 * Also, within the requests any handle values with the MSB set (0x80000000 to 0xFFFFFFFF)
 * are looked up in this small handle table before looking up in the global handle tree.
 *
 * At the completion of instantiating a widget table, the number in handle table slot 0
 * is returned to the client, and should generally be the head widget in the widget tree
 * defined by this template.
 *
 */

#endif /* __WT_H */
