/* $Id$
  *
  * kbfile.c - Functions to validate and load patterns from a keyboard file
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

#include <stdio.h>
#include <stdlib.h>
#include <picogui.h>
#include <netinet/in.h>
#include "kbfile.h"

/* as some systems don't have htons primitives (as uClinux) */
#ifdef _NEED_NTOHS_FRIENDS_
    #include "ntohs_fr.h"
#else
    #include <netinet/in.h>
#endif

/* Data about all the patterns read from the keyboard definition file */
static struct mem_pattern pat;

/* All pattern information */
static struct pattern_info * pattern_data = NULL;

/* Current pattern info */
static struct pattern_info * current_pat = NULL;

/* Validate a pattern's header, read the file data in memory, */
/* fill in global data for mem_pattern */
unsigned char * kb_validate(FILE *f, struct mem_pattern ** user_pat) {
   unsigned long checksum;
   struct keyboard_header hdr;
   int i;
   unsigned char * file_buffer;

   /* Clear pattern info */
   memset (&pat, 0, sizeof (pat));

   /* Read in the header */
   rewind(f);
   if (!fread(&hdr,1,sizeof(hdr),f))
     return NULL;
   
   /* Check the magic */
   if (hdr.magic[0] != 'P' ||
       hdr.magic[1] != 'G' ||
       hdr.magic[2] != 'k' ||
       hdr.magic[3] != 'b')
     return NULL;
   
   /* Re-byteorder-ize the header */
   hdr.file_len = ntohl(hdr.file_len);
   hdr.file_sum32 = ntohl(hdr.file_sum32);
   hdr.file_ver = ntohs(hdr.file_ver);
   hdr.num_patterns = ntohs(hdr.num_patterns);
   hdr.app_size = ntohs(hdr.app_size);
   hdr.app_sizemode = ntohs(hdr.app_sizemode);
   hdr.app_side = ntohs(hdr.app_side);
   
   /* Check file version */
   if (hdr.file_ver > PGKB_FORMATVERSION)
     return NULL;
   if (hdr.file_ver < PGKB_MINFORMATVERSION)
     return NULL;
   
   /* Measure file length, compare it */
   fseek(f,0,SEEK_END);
   if (ftell(f)!=hdr.file_len)
     return NULL;

   /* Read the file in memory */
   rewind(f);
   file_buffer = (unsigned char *) malloc (sizeof (unsigned char) * hdr.file_len);
   if (!file_buffer)
     return NULL;
   fread (file_buffer, sizeof (unsigned char), hdr.file_len, f);
   
   /* Measure the file's checksum */
   checksum = 0;
   for (i = 0; i < hdr.file_len; i++, file_buffer++)
     {
       checksum += *file_buffer;
     }
   file_buffer -= hdr.file_len;
   /* Don't count the checksum itself */
   checksum -= file_buffer [8] + file_buffer [9] + file_buffer [10] + file_buffer [11];
   if (checksum!=hdr.file_sum32)
     {
       free (file_buffer);
       return NULL;
     }

   /* Looks ok, store data */
   pat.num_patterns = hdr.num_patterns;
   pat.app_size = hdr.app_size;
   pat.app_sizemode = hdr.app_sizemode;
   pat.app_side = hdr.app_side;

   /* Affect user mem_pattern variable */
   (*user_pat) = &pat;

   return file_buffer;
}

/* Load (and allocate memory for if necessary) all patterns from file data */
/* Return nonzero on error */
int kb_loadpatterns (unsigned char * file_buffer)
{
   /* Number of current pattern */
   int npat;
   /* Current pattern */
   struct pattern_info * current_pat;
   
   /* Skip the header */
   file_buffer += sizeof (struct keyboard_header);
   
   /* Allocate space for the patterns */
   pattern_data = (struct pattern_info *) 
     malloc (pat.num_patterns * sizeof (struct pattern_info));
   if (!pattern_data)
     return 1;
   current_pat = pattern_data;

   /* Read all the patterns */
   for (npat = 0; npat < pat.num_patterns; npat++)
     {
       struct pattern_header pat_hdr;
       /* Current key */
       struct key_entry * current_key;

       /* Read pattern header */
       memcpy (&pat_hdr, file_buffer, sizeof (pat_hdr));
       file_buffer += sizeof (pat_hdr);

       /* Byte-swap data */
       pat_hdr.canvasdata_len = ntohl (pat_hdr.canvasdata_len);
       pat_hdr.num_requests   = ntohs (pat_hdr.num_requests);
       pat_hdr.num_keys       = ntohs (pat_hdr.num_keys);
       
       current_pat->canvasdata_len = pat_hdr.canvasdata_len;
       current_pat->num_keys = pat_hdr.num_keys;

       if (current_pat->num_keys > 0)
	 {
	   current_pat->ptype = PGKB_REQUEST_NORMAL;

	   /* Read canvas data */
	   current_pat->canvas_buffer = (char *) malloc (current_pat->canvasdata_len);
	   if (!current_pat->canvas_buffer)
	     return 1;
	   memcpy (current_pat->canvas_buffer, file_buffer, current_pat->canvasdata_len * sizeof (char));
	   file_buffer += current_pat->canvasdata_len * sizeof (char);

	   /* Read requests */
	   for ( ;pat_hdr.num_requests > 0; pat_hdr.num_requests--)
	     {
	       struct request_header kbrqh;
	       struct pgrequest req;
	       char * req_buf;
	       pghandle result;

	       /* Read request header */
	       memcpy (&kbrqh, file_buffer, sizeof (kbrqh));
	       file_buffer += sizeof (kbrqh);

	       /* Byte-swap data */
	       kbrqh.canvasdata_offset = ntohl (kbrqh.canvasdata_offset);

	       /* Validate offset */
	       if (kbrqh.canvasdata_offset > (pat_hdr.canvasdata_len - 4))
		 return 1;

	       /* Read request */
	       memcpy (&req, file_buffer, sizeof (req));
	       file_buffer += sizeof (req);

	       /* Byte-swap data */
	       req.type = ntohs (req.type);
	       req.size = ntohl (req.size);

	       /* Read request data */
	       req_buf = (char *) file_buffer;
	       file_buffer += req.size;

	       /* Evaluate request */
	       result = pgEvalRequest (req.type, req_buf, req.size);
	   
	       /* Link request to canvas data */
	       *( (unsigned long *) 
		  (current_pat->canvas_buffer + kbrqh.canvasdata_offset) 
		  ) = htonl (result);
	     }

	   /* Read keys */
	   current_pat->keys = (struct key_entry *)
	     malloc (pat_hdr.num_keys * sizeof (struct key_entry));
	   if (!current_pat->keys)
	     return 1;
	   memcpy (current_pat->keys, file_buffer, pat_hdr.num_keys * sizeof (struct key_entry));
	   file_buffer += pat_hdr.num_keys * sizeof (struct key_entry);

	   /* Byte-swap key data */
	   current_key = current_pat->keys;
	   for ( ;pat_hdr.num_keys > 0; pat_hdr.num_keys--)
	     {
	       current_key->x       = ntohs (current_key->x);
	       current_key->y       = ntohs (current_key->y);
	       current_key->w       = ntohs (current_key->w);
	       current_key->h       = ntohs (current_key->h);
	       current_key->flags   = ntohl (current_key->flags);
	       current_key->key     = ntohs (current_key->key);
	       current_key->pgkey   = ntohs (current_key->pgkey);
	       current_key->mods    = ntohs (current_key->mods);
	       current_key->pattern = ntohs (current_key->pattern);
	   
	       current_key++;
	     }

	 }
       else if (pat_hdr.num_requests = PGKB_REQUEST_EXEC)
	 {
	   current_pat->ptype = PGKB_REQUEST_EXEC;
	   /* Read raw data */
	   current_pat->canvas_buffer = (char *) malloc (current_pat->canvasdata_len + 1);
	   if (!current_pat->canvas_buffer)
	     return 1;
	   memcpy (current_pat->canvas_buffer, file_buffer, current_pat->canvasdata_len * sizeof (char));
	   file_buffer += current_pat->canvasdata_len * sizeof (char);
	   current_pat->canvas_buffer[current_pat->canvasdata_len] = 0;
	 }
       else
	 {
	   return 1;
	 }
       current_pat++;
     }

   return 0;
}

/* execute a command in a subprocess */
void spawn_process (const char *command)
     {
       pid_t pid;
     
       pid = fork ();
       if (pid == 0)
         {
           /* This is the child process.  Execute the shell command. */
           execl ("/bin/sh", "/bin/sh", "-c", command, NULL);
           _exit (EXIT_FAILURE);
         }
     }


/* Select a pattern from the ones loaded in memory, and load it into the
   specified canvas widget */
void kb_selectpattern (unsigned short pattern_num, pghandle canvas)
{
  /* Flag indicating if we are within a context */
  static int inContext = 0;
  struct pattern_info * requested_pat = NULL;

  if (pattern_num < pat.num_patterns)
    {
      requested_pat = pattern_data + pattern_num;

      if (requested_pat->ptype == PGKB_REQUEST_NORMAL)
	{
	  /* Manage context */
	  if (inContext)
	    {
	      pgLeaveContext ();
	    }
	  else
	    {
	      inContext = 1;
	    }
	  pgEnterContext ();

          current_pat = requested_pat;

	  pgWriteData (canvas, pgFromMemory (current_pat->canvas_buffer, 
					     current_pat->canvasdata_len));
	  pgWriteCmd (canvas, PGCANVAS_REDRAW, 0);
	  pgSubUpdate (canvas);
	}
      else if (requested_pat->ptype = PGKB_REQUEST_EXEC)
	{
	  spawn_process (requested_pat->canvas_buffer);
	}
    }
}

/* Find the key in the current pattern given the clicked coordinates */
struct key_entry * find_clicked_key (unsigned int x, unsigned int y)
{
  struct key_entry * key;
  struct key_entry * clicked_key = NULL;
  int i;

  if (current_pat)
    {
      for (i = 0, key = current_pat->keys; i < current_pat->num_keys; i++, key++)
	{
	  if (x < key->x) continue;
	  if (x > key->x + key->w - 1) continue;
	  if (y < key->y) continue;
	  if (y > key->y + key->h - 1) continue;
	  
	  /* Key found */
	  clicked_key = key;
	  break;
	}
    }

  return clicked_key;
}

/* The End */
