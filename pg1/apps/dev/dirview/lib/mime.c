/* $Id$
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
 */

#include <stdio.h>
#include <stddef.h>
#include <regex.h>
#include <string.h>

#define DECLARE_MIME_TYPE_DESC
#include "mime.h"
#include "debug.h"

/*****************************************************************************/

static int
match(const char* name, const char* pat_list, int is_ext)
{
  char* pat;
  const char* src;
  char pat_buf[100];

  /* no pattern => no good */
  if(pat_list==0 || strlen(pat_list)==0) return 0;

  /* copy the pattern list in the buffer so that we can tokenize it */
  strncpy(pat_buf, pat_list, sizeof(pat_buf)-1);
  pat_buf[sizeof(pat_buf)-1] = 0;

  /* take just the extension or the whole object name as source */
  src = is_ext ? strrchr(name, '.') : name;

  /* no extension or no object name or => no good */
  if(src==0) return 0;

  /* do for each pattern (using the last one first) ... */
  while(strlen(pat_buf)) {
    /* extract last pattern and zero-terminate predecessor */
    pat = (char*)strrchr(&pat_buf[0],' ');
    if(pat) {
      *pat = '\0';
      ++pat; /* skip separating space */
    }
    else pat = pat_buf;

    /* check for extension match... */
    if(is_ext) {
      if(!strcmp(src+1, pat)) return 1;
    }
    /* ...or for regex match */
    else {
      int match = 0;
      regex_t preg;
      if (regcomp(&preg, pat, REG_EXTENDED)) {
	WARNF("libpgdirview: "__FILE__": warning: could not compile regex\n");
      }
      else {
	/* consider substring, but do not care where they are */
	if(!regexec(&preg, src, 0, NULL, 0)) {
	  match = 1;
	}
	regfree(&preg);
	if(match) return 1;
      }
    }
    
    /* check if we have reached the beginning of the pattern list */
    if(pat==pat_buf) break;
  }
  return 0;
}

/*****************************************************************************/

LpgdvMimeTypeId
lpgdv_mime_type_id_of(const char* obj_name, int fd)
{
  if(obj_name && strlen(obj_name)) {
    int i;
    LpgdvMimeTypeDesc* desc;
    for(i=0, desc=MimeTypeDesc;
	i<sizeof(MimeTypeDesc)/sizeof(LpgdvMimeTypeDesc);
	++i, ++desc) {

      if( match(obj_name, desc->obj_ext, 1) ||
	  match(obj_name, desc->obj_regex, 0) ) {
	return i;
      }
    }
  }

  if(fd!=-1) {
    /* not yet implemented */
  }

  return MIME_UNKNOWN;
}

/*****************************************************************************/

const char*
lpgdv_mime_type_name_of(LpgdvMimeTypeId id)
{
  if(id>=0 && id<sizeof(MimeTypeDesc)/sizeof(LpgdvMimeTypeDesc)) {
    return MimeTypeDesc[id].mt_name;
  }
  else return "<unknown>";
}

/*****************************************************************************/
