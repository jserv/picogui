/* $Id: kbfile.c,v 1.1 2001/05/02 04:37:57 micahjd Exp $
  *
  * kbfile.c - Functions to validate and load patterns from a keyboard file
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

#include <stdio.h>
#include <picogui.h>
#include "kbfile.h"

/* Validate a pattern's header, fill in global data for mem_pattern */
int kb_validate(FILE *f, struct mem_pattern *pat) {
   unsigned long checksum = 0;
   unsigned char c;
   struct keyboard_header hdr;
   int i;
   
   /* Read in the header */
   rewind(f);
   if (!fread(&hdr,1,sizeof(hdr),f))
     return 1;
   
   /* Check the magic */
   if (hdr.magic[0] != 'P' ||
       hdr.magic[1] != 'G' ||
       hdr.magic[2] != 'k' ||
       hdr.magic[3] != 'b')
     return 1;
   
   /* Re-byteorder-ize the header */
   hdr.file_len = ntohl(hdr.file_len);
   hdr.file_sum32 = ntohl(hdr.file_sum32);
   hdr.vw = ntohs(hdr.vw);
   hdr.vh = ntohs(hdr.vh);
   hdr.file_ver = ntohs(hdr.file_ver);
   hdr.num_patterns = ntohs(hdr.num_patterns);
   
   /* Check file version */
   if (hdr.file_ver > PGKB_FORMATVERSION)
     return 1;
   
   /* Measure file length, compare it */
   fseek(f,0,SEEK_END);
   if (ftell(f)!=hdr.file_len)
     return 1;
   
   /* Measure the file's checksum */
   rewind(f);
   for (i=8;i;i--) {
      fread(&c,1,1,f);
      checksum += c;
   }
   /* Skip the checksum itself */
   fseek(f,4,SEEK_CUR);
   while (fread(&c,1,1,f))
     checksum += c;
   if (checksum!=hdr.file_sum32)
     return 1;

   /* Looks ok, store data */
   pat->vw = hdr.vw;
   pat->vh = hdr.vh;
   pat->num_patterns = hdr.num_patterns;
   
   return 0;
}

/* Load (and allocate memory for if necessary) a pattern from file.
 * This keeps the key table in memory, and loads the pattern itself
 * into the specified canvas widget 
 *
 * Normally I wouldn't like loading things like this from file, but on
 * a handheld it's really flash memory or ram anyway, and a machine with
 * a magnetic disk probably has enough cache to make it alright. At the moment
 * it doesn't seem worth storing the entire pattern table in memory.
 */
int kb_loadpattern(FILE *f, struct mem_pattern *pat,
		   short patnum, pghandle canvas) {
   struct pattern_header pathdr;
   struct pgrequest rqh;
   unsigned long x;
   struct key_entry *k;
   
   /* If necessary, free the previous key table */
   if (pat->keys)
     free(pat->keys);
   
   /* Seek to immediately after the header */
   fseek(f,sizeof(struct keyboard_header),SEEK_SET);

   /* Skip patterns */
   for (;patnum;patnum--) {

      /* Load and byte-swap the pattern header */
      if (!fread(&pathdr,1,sizeof(pathdr),f))
	return 1;
      pathdr.canvasdata_len = ntohl(pathdr.canvasdata_len);
      pathdr.num_requests   = ntohs(pathdr.num_requests);
      pathdr.num_keys       = ntohs(pathdr.num_keys);
      
      /* Skip canvas data block */
      fseek(f,pathdr.canvasdata_len,SEEK_CUR);
      
      /* Skip requests */
      for (;pathdr.num_requests;pathdr.num_requests--) {
	 
	 /* Skip keyboard request header */
	 fseek(f,sizeof(struct request_header),SEEK_CUR);
	 
	 /* Read PicoGUI request header and skip request */
	 fread(&rqh,1,sizeof(rqh),f);
	 fseek(f,ntohl(rqh.size),SEEK_CUR);
      }
      
      /* Skip keys */
      fseek(f,sizeof(struct key_entry) * pathdr.num_keys,SEEK_CUR);
   }

   /* Read the selected pattern */

   /* Load and byte-swap the pattern header */
   if (!fread(&pathdr,1,sizeof(pathdr),f))
     return 1;
   pathdr.canvasdata_len = ntohl(pathdr.canvasdata_len);
   pathdr.num_requests   = ntohs(pathdr.num_requests);
   pathdr.num_keys       = ntohs(pathdr.num_keys);
   pat->num_keys = pathdr.num_keys;
   
   /* Reset the canvas */
   pgWriteCmd(canvas,PGCANVAS_NUKE,0);
   pgWriteCmd(canvas,PGCANVAS_GROP,6,PG_GROP_SETMAPPING,
	      0,0,pat->vw,pat->vh,PG_MAP_SCALE);
   
   /* Load the canvasdata block into the canvas */
   pgWriteData(canvas,pgFromStream(f,pathdr.canvasdata_len));
   
   /* FIXME: load requests here! */
   
   /* Load key table into memory */
   if (pat->num_keys) {
      x = sizeof(struct key_entry) * pat->num_keys;
      pat->keys = (struct key_entry *) malloc(x);
      if (!pat->keys)
	return 1;
      if (!fread(pat->keys,1,x,f))
	return 1;
      
      /* Byteswap the key entries */
      k = pat->keys;
      x = pat->num_keys;
      for (;x;x--,k++) {
	 k->x = ntohs(k->x);
	 k->y = ntohs(k->y);
	 k->w = ntohs(k->w);
	 k->h = ntohs(k->h);
	 k->flags = ntohl(k->flags);
	 k->key = ntohs(k->key);
	 k->pgkey = ntohs(k->pgkey);
	 k->mods = ntohs(k->mods);
	 k->pattern = ntohs(k->pattern);
      }
   }
      
   return 0;
}

/* The End */
