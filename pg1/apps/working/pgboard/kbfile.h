/* $Id$
  *
  * kbfile.h - Definition of the PicoGUI keyboard file format 
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

/************** File format */

#define PGKB_FORMATVERSION 4

/* if you make changes that may break backwards-compatibility, make this
 * equal to PGKB_FORMATVERSION
 *
 * pgboard compiled for your version will then refuse to load files with
 * FORMATVERSION lower than this
 */

#define PGKB_MINFORMATVERSION 0

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
   unsigned short app_side;     /* Application size/position params */
   unsigned short app_size;
   unsigned short app_sizemode;

   unsigned short dummy;        /* Padding */
};

/*
 * A "normal" pattern includes a pattern header, a block of canvas command data,
 * a table of requests to load, and a table of keys
 * 
 * - pattern header
 * - canvas data
 * - request table
 *   ...
 * - key table
 *   ...
 * 
 *
 * A special pattern has 0 keys in the pattern header, and num_requests is
 * instead a constant with the pattern type. Instead of the canvas data
 * it has the raw data corresponding to the pattern data
 * (eg a string for EXEC patterns)
 *
 */

struct pattern_header {
   unsigned long canvasdata_len;   /* length (in bytes) of canvas data block */
   unsigned short num_requests;
   unsigned short num_keys;
};

#define PGKB_REQUEST_NORMAL	 0
#define PGKB_REQUEST_EXEC	 1

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

/* The special pattern header defines the kind of pattern this is */

struct special_pattern_header {
   unsigned long ptype;
   unsigned long data_len;
};

/************** Keyboard loading functions and in-memory representation */

struct pattern_info
{
  unsigned short ptype;
  unsigned long canvasdata_len;
  char * canvas_buffer;
  unsigned short num_keys;
  struct key_entry * keys;
};

struct mem_pattern {
   /* Filled in during kb_validate */
   unsigned short num_patterns;
   unsigned short app_side;
   unsigned short app_size;
   unsigned short app_sizemode;
};


/* Validate a pattern's header, read the file data in memory, */
/* fill in global data for mem_pattern */
unsigned char * kb_validate(FILE *f, struct mem_pattern ** pat);

/* Load (and allocate memory for if necessary) all patterns from file data */
/* Return nonzero on error */
int kb_loadpatterns (unsigned char * file_buffer);

/* Select a pattern from the ones loaded in memory, and load it into the
   specified canvas widget */
void kb_selectpattern (unsigned short pattern_num, pghandle canvas);

/* Find the key in the current pattern given the clicked coordinates */
struct key_entry * find_clicked_key (unsigned int x, unsigned int y);

/* The End */
