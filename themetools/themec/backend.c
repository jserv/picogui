/* $Id: backend.c,v 1.2 2000/09/25 19:41:19 micahjd Exp $
 *
 * backend.c - convert the in-memory representation of the
 *             theme data to the actual compiled theme file
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include "themec.h"
#include "y.tab.h"

/* The theme heap */

unsigned long themeheap_size;
unsigned char *themeheap;
unsigned char *themeheap_p;   /* Current position */

/* Important structures in the theme heap */
struct pgtheme_header *themehdr;
struct pgtheme_thobj  *thobjarray;

/* Error reporting in the backend */
void beerror(const char *s);

/* qsort comparison functions */
int compare_thobj(struct pgtheme_thobj *a,struct pgtheme_thobj *b);

/********* Back end code ********/

void backend(void) {
  struct objectnode *op;
  struct propnode *pp;
  long c;
  struct pgtheme_thobj *thop;

  /* Calculate the theme heap's size, and allocate it */
  themeheap_size = 
    sizeof(struct pgtheme_header) +
    sizeof(struct pgtheme_thobj) * num_thobj +
    sizeof(struct pgtheme_prop) * num_totprop +
    sizeof(struct pgtheme_tags) * num_tags +
    datasz_loader +
    datasz_tags;

  themeheap_p = themeheap = malloc(themeheap_size);
  if (!themeheap)
    beerror("Error allocating theme heap");
  memset(themeheap,0,themeheap_size);

  /* Reserve space for the header, and fill in
     what we can now. Do the checksum later */

  themehdr = (struct pgtheme_header *) themeheap_p;
  themeheap_p += sizeof(struct pgtheme_header);
  
  strcpy(themehdr->magic,"PGth");   /* It's all 0 anyway, so ignore the null */
  themehdr->file_len = htonl(themeheap_size);
  themehdr->file_ver = htons(PGTH_FORMATVERSION);
  themehdr->num_tags = htons(num_tags);
  themehdr->num_thobj = htons(num_thobj);
  themehdr->num_totprop = htons(num_totprop);

  /* Reserve the theme object array */
  thobjarray = (struct pgtheme_thobj *) themeheap_p;
  themeheap_p += sizeof(struct pgtheme_thobj) * num_thobj;  

  /* Transcribe the thobj linked list into the array */
  op = objectlist;
  thop = thobjarray;
  while (op) {
    thop->id = htons(op->id);

    /* Count the properties */
    c = 0;
    pp = op->proplist;
    while (pp) {
      c++;
      pp = pp->next;
    }
    thop->num_prop = htons(c);

    /* Link the property lists later */

    op = op->next;
    thop++;
  }

  /* Sort the thobj array by id, in ascending order. This
     is required by the format so the server can use a 
     binary search on the arrays. */
  qsort(thobjarray,num_thobj,sizeof(struct pgtheme_thobj),
  	&compare_thobj);
}

/********* Utility functions ********/

void beerror(const char *s) {
  fprintf(stderr,"Back end internal error: %s\n",s);
  exit(2);
}

int compare_thobj(struct pgtheme_thobj *a,struct pgtheme_thobj *b) {
  if (ntohs(a->id) < ntohs(b->id))
    return -1;
  if (ntohs(a->id) > ntohs(b->id))
    return 1;
  beerror("duplicate theme objects");
}

/* The End */
