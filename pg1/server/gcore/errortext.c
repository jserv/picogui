/* $Id$
 *
 * errortext.c - optional error message strings
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
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

#include <pgserver/common.h>

#include <stdio.h>              /* For file IO */
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Maximum line size when reading error tables */
#define LINESIZE     256

/* A loadable error table */
char **loaded_errors;
int    num_loaded_errors;

/* Builtin error table */
static const char *builtin_errors[] = {
#ifdef CONFIG_TEXT          /* Normal error table */
# include "defaulttext.inc"
#else                       /* Minimalist error table */
# include "tinytext.inc"
#endif
};

/* Return an error string for the given error code.
 * 
 * Search for a string in this order:
 * 1. Loaded error table 
 * 2. Builtin error table
 * 3. Numeric code
 *
 */
const char *errortext(g_error e) {
  const char *errtxt = NULL;
  int errnum = (e & 0xFF) - 1;
  static char errbuf[20];	/* rely on this being 0-initialized */

  /* Loaded table */
  if (loaded_errors && (errnum < num_loaded_errors))
    errtxt = loaded_errors[errnum];
  if (errtxt)
    return errtxt;

  /* Builtin table */
  if (errnum < sizeof(builtin_errors)/sizeof(char *))
    errtxt = builtin_errors[errnum];
  if (errtxt)
    return errtxt;
   
  /* Nothing left to do but give the raw numeric error code */
  snprintf(errbuf,sizeof(errbuf)-1,"Error#%d/%d",e>>8,e & 0xFF);
  return errbuf;
}

/* Load an internationalized error table from disk */
g_error errorload(const char *filename) {
  g_error e;
  FILE *f;
  char line[LINESIZE+1];
  int totalsize = 0;
  int n;
  char *p,*q;
  char *errorheap;

  /* Free any existing error table */
  if (loaded_errors)
    g_free(loaded_errors);
  loaded_errors = NULL;
  num_loaded_errors = 0;
  if (!filename)
    return success;

  f = fopen(filename,"r");
  if (!f) 
    return mkerror(PG_ERRT_IO,21);      /* Can't open error file */

  /* Now we need to read the file twice. First count the total size
   * of all strings and the maximum error number. Then allocate the necessary
   * memory block and read in the text */

  while (fgets(line,LINESIZE,f)) {
    n = strtol(line,&p,0);         /* Error code */
    if (!n || p==line)             /* skip blank lines or comments */
      continue;
    if (n>num_loaded_errors)       /* Store maximum error code */
      num_loaded_errors = n;
    while (p && isspace(*p)) {     /* Skip whitespace */
      p++;
    }
    totalsize += strlen(p);        /* Accumulate string size */
  }

  rewind(f);

  /* Allocate buffer */
  e = g_malloc((void**) &loaded_errors,
	       totalsize + sizeof(char*)*num_loaded_errors);
  if (iserror(e))
    fclose(f);
  errorcheck;
  memset(loaded_errors,0,totalsize + sizeof(char*)*num_loaded_errors);
  errorheap = ((char*)loaded_errors) + sizeof(char*)*num_loaded_errors;

  /* Now read in the file for real */
  while (fgets(line,LINESIZE,f)) {
    n = strtol(line,&p,0);         /* Error code */
    if (!n || p==line)             /* skip blank lines or comments */
      continue;
    loaded_errors[n-1] = errorheap;/* Store string pointer */
    while (p && isspace(*p)) {     /* Skip whitespace */
      p++;
    }
    p[strlen(p)-1] = 0;            /* Cut off newline */

    /* Convert escape sequences in the string */
    q=p;
    while (*q) {
      if ( (*q == '\\') && *(q+1) ) {
	switch (*(q+1)) {
	  
	case 'n':
	  *q = '\n';
	  break;

	case 't':
	  *q = '\t';
	  break;

	default:
	  *q = *(q+1);

	}

	/* Move the rest of the string to cover up the hole */
	memmove(q+1,q+2,strlen(q+1));
      }
      q++;
    }

    strcpy(errorheap,p);           /* Toss the string on the heap */
    errorheap += strlen(p)+1;
  }

  fclose(f);
  return success;
}

/* The End */




