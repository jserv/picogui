/* $Id: kbfile.h,v 1.3 2001/05/04 23:27:29 micahjd Exp $
  *
  * kbfile.h - Definition of the PicoGUI keyboard file format 
  * 
  * PicoGUI small and efficient client/server GUI
  * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

/************** File format */

#define PGKB_FORMATVERSION  0x0002

/* A keyboard file is made of a header then one or more patterns:
 * (The first pattern in the file is taken as the default)
 * 
 * - keyboard header
 * - pattern1
 * - pattern2
 *   ...
 * 
 */

struct keyboard_header {
   char magic[4];               /* = "PGkb" */
   
   unsigned long  file_len;     /* Expected total file length */
   unsigned long  file_sum32;   /* 32-bit checksum of entire file except
				 * these 4 bytes */

   unsigned short file_ver;     /* Format version */
   
   unsigned short num_patterns;
};

/*
 * Every pattern includes a pattern header, a block of canvas command data,
 * a table of requests to load, and a table of keys
 * 
 * - pattern header
 * - canvas data
 * - request table
 *   ...
 * - key table
 *   ...
 * 
 */

struct pattern_header {
   unsigned long canvasdata_len;   /* length (in bytes) of canvas data block */
   unsigned short num_requests;
   unsigned short num_keys;
};

/* Every request consists of the following request header, followed by a
 * standard PicoGUI request packet */

struct request_header {
   unsigned long canvasdata_offset; /* Offset in canvas data block to stick
				     * the return value of this request */
};

/* The key table is an array of these structures */

struct key_entry {
   unsigned short x,y,w,h;          /* Hotspot */
   unsigned long flags;
   unsigned short key;              /* Ascii/unicode key value */
   unsigned short pgkey;            /* PGKEY value */
   unsigned short mods;             /* Key modifiers */
   unsigned short pattern;          /* Pattern to jump to */
};

/************** Keyboard loading functions and in-memory representation */

struct mem_pattern {
   /* Filled in during kb_validate */
   unsigned short num_patterns;

   /* Filled in during kb_loadpattern */
   unsigned short num_keys;
   struct key_entry *keys;
};

/* These functions return nonzero on error */

/* Validate a pattern's header, fill in global data for mem_pattern */
int kb_validate(FILE *f, struct mem_pattern *pat);

/* Load (and allocate memory for if necessary) a pattern from file.
 * This keeps the key table in memory, and loads the pattern itself
 * into the specified canvas widget */
int kb_loadpattern(FILE *f, struct mem_pattern *pat,
		   short patnum, pghandle canvas);

/* The End */
