/* $Id$
 *
 * backend.c - convert the in-memory representation of the
 *             theme data to the actual compiled theme file
 *
 * MAGIC compiler
 * Magic Algorithm for General Interface Configurability
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
#include "pgtheme.h"

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
int compare_prop(struct pgtheme_prop *a,struct pgtheme_prop *b);
int compare_thobj(struct pgtheme_thobj *a,struct pgtheme_thobj *b);

/********* Back end code ********/

void backend(void) {
  struct objectnode *op;
  struct propnode *pp;
  unsigned long c;
  unsigned char *cp;
  struct pgtheme_thobj *thop;
  struct pgtheme_prop  *propp,*proparray;
  struct loadernode *ldr;
  unsigned long sum=0;

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
    op->num_prop   = c;

    /* Set up for linking */
    op->proplist->link_from = (unsigned long*) &thop->proplist;

    op = op->next;
    thop++;
  }

  /* Reserve, transcribe, and sort all the property lists */
  op = objectlist;
  while (op) {  /* For every theme object ... */

    /* Allocate the property array in the theme heap */
    proparray = propp = (struct pgtheme_prop *) themeheap_p;
    themeheap_p += sizeof(struct pgtheme_prop) * op->num_prop;  
    
    pp = op->proplist;
    while (pp) {  /* and every property... */
      
      /* Transcribe into the array */
      propp->id = htons(pp->propid);
      propp->loader = ntohs(pp->loader);
      propp->data = ntohl(pp->data);

      /* Set up for loader linking */
      if (pp->ldnode)
	pp->ldnode->link_from = (unsigned long*) &propp->data;

      /* Link */
      if (pp->link_from)
	*pp->link_from = htonl( ((char*)propp) - ((char*)themeheap) );

      propp++;
      pp = pp->next;
    }

    /* Next! */
    op = op->next;
  }		     
  
  /* Append and link loaders */
  ldr = loaderlist;
  while (ldr) {

    /* Align on a word boundary */
    if ( ( ((char*)themeheap_p) - ((char*)themeheap) ) & 3)
       themeheap_p += 4 - (( ((char*)themeheap_p) - ((char*)themeheap) ) & 3);
     
    /* link */
    *ldr->link_from = htonl( ((char*)themeheap_p) - ((char*)themeheap) );

    /* Copy to the heap */
    memcpy(themeheap_p,ldr->data,ldr->datalen);
    themeheap_p += ldr->datalen;  

    ldr = ldr->next;
  }

  /* Sort everything after all linking is done */

  /* Property lists */
  thop = thobjarray;
  for (c=0;c<num_thobj;c++,thop++) {
    proparray = (struct pgtheme_prop *) (((char*)themeheap) + 
					 ntohl(thop->proplist));

    /* Sort the property list in ascending order, so
       the server can binary search it */
    qsort(proparray,ntohs(thop->num_prop),sizeof(struct pgtheme_prop),
	  &compare_prop);
  }

  /* Sort the thobj array by id, in ascending order. This
     is required by the format so the server can use a 
     binary search on the arrays. */
  qsort(thobjarray,num_thobj,sizeof(struct pgtheme_thobj),
  	&compare_thobj);

  /* Calculate the checksum.
   * The theme file's checksum is taken by adding
   * all the bytes in the file except the
   * portion containing the checksum itself. Here we
   * don't worry about that because the checksum field
   * is set to zero.
   */

  cp = (char *) themeheap;
  for (c=0;c<themeheap_size;c++)
    sum += *(cp++);

  themehdr->file_sum32 = htonl(sum);
  
}

/********* Utility functions ********/

void beerror(const char *s) {
  fprintf(stderr,"Back end internal error: %s\n",s);
  exit(2);
}

int compare_prop(struct pgtheme_prop *a,struct pgtheme_prop *b) {
  if (ntohs(a->id) < ntohs(b->id))
    return -1;
  if (ntohs(a->id) > ntohs(b->id))
    return 1;
  beerror("duplicate properties");
}

int compare_thobj(struct pgtheme_thobj *a,struct pgtheme_thobj *b) {
  if (ntohs(a->id) < ntohs(b->id))
    return -1;
  if (ntohs(a->id) > ntohs(b->id))
    return 1;

  if (ntohs(a->id) != PGTH_O_CUSTOM)
    beerror("duplicate theme objects");

  return 0;
}

/* The End */
