/* $Id$
 * 
 * theme.h - Defines the theme file format, used by the server and by
 *           programs that read and write theme files
 *
 * MAGIC format
 * Magic Algorithm for General Interface Configurability
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

#ifndef __THEME_H
#define __THEME_H

/* format version (file_ver) */
#define PGTH_FORMATVERSION  0x0002

/******** PicoGUI theme file format ********/
/* All numbers in network byte order */

/* Every theme starts with this header (20 bytes) */
struct pgtheme_header {
  char magic[4];              /* = "PGth" */ 

  u32 file_len;    /* Expected file length */
  u32 file_sum32;  /* 32-bit checksum */

  u16 file_ver;    /* Format version */

  u16 num_tags;    /* Number of (optional) tags defined */
  u16 num_thobj;   /* Number of theme objects defined */
  u16 num_totprop; /* Total number of properties defined */
};

/* Next, there is an array of num_thobj theme objects, sorted in
   ascending order by their id... (8 bytes) */
struct pgtheme_thobj {
  u16 id;         /* A PGTH_O_* constant */
  u16 num_prop;   /* Number of properties in this theme object */
  u32 proplist;   /* File offset of the object's property list */
};

/* The proplist pointed to by the theme objects is an array of these
   property structures (8 bytes) */
struct pgtheme_prop {
  u16 id;         /* a PGTH_P_* constant */
  u16 loader;     /* a PGTH_LOAD_* constant, specifying any
				necessary load-time preprocessing for
				the data */
  u32 data;       /* The actual data content of the property */
};

/* Only the information above this point is actually stored in th
   server's theme heap. The file may contain more information used
   by the loaders, but it is stored seperately, such as in
   a handle. */

/* This is an element in an optional array of theme tags that occurs
 * immediately after the thobj table. It stores information
 * not necessarily needed at runtime, like the theme's author,
 * web site, e-mail address, or other information.
 */
struct pgtheme_tags {
  u16 id;         /* a PGTH_TAG_* constant */
  u16 len;        /* Length of following data */
  /* followed by 'len' bytes of data */
};

#endif /* __THEME_H */

/* The End */




