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

#include <stdlib.h>
#include <string.h>
#include "protocol.h"
#include "debug.h"

/*****************************************************************************/

extern LpgdvProtocol file_protocol;

const LpgdvProtocol *protocols[] = {
  &file_protocol,
  0
};

/*****************************************************************************/

const LpgdvProtocol*
lpgdv_protocol_find(const char* name)
{
  int i;
  const LpgdvProtocol *prot;

  if(name) {
    for(i=0; prot=protocols[i]; ++i) {
      if(!strcasecmp(prot->name, name))
	return prot;
    }
  }
  return 0;
}

/*****************************************************************************/

int lpgdv_protocol_split_url(const char* url,
			     const LpgdvProtocol* prot,
			     LpgdvSplittedUrl* su)
{
  ENTER("lpgdv_protocol_split_url");
  char* url_copy = 0;
  char* p;
  char* login = 0;
  char* host = 0;
  int err = 1;

  if(su) memset(su, 0, sizeof(LpgdvSplittedUrl));
  if(su==0 || url==0 || strlen(url)==0) goto done;

  if(prot && prot->has_host) {
    if(strncmp(url, "//", 2))  goto done; /* invalid url */
    else if(strlen(url)>=2) url+=2;
    else goto done; /* invalid url */
  }

  url_copy = strdup(url);
  if(url_copy==0) {
    WARNF("warning: strdup(url) failed\n");
    goto done;
  }

  err = 0;

  /* extract arguments */
  p = (prot==0 || prot->has_arguments) ? strchr(url_copy, '?') : 0;
  if(p) {
    *p = '\0';
    su->arguments = strdup(p+1);
    if(su->arguments==0) err |= 1;
  }

  /* extract path */
  if(prot && !prot->has_host && !prot->has_login) p = url_copy;
  else p = strchr(url_copy, '/');
  if(p) {
    char* z = p;
    if(!strncmp(p, "//", 2)) p+=2;
    su->path = strdup(p);
    *z = '\0';
    if(su->path==0) err |= 1;
  }

  /* split login from host */
  if(prot==0 || prot->has_login) {
    p = strchr(url_copy, '@');
    if(p) {
      *p = '\0';
      login = url_copy;
      host = p+1;
    }
    else {
      login = 0;
      host = url_copy;
    }
  }

  /* extract host name and port */
  su->port = prot ? prot->default_port : 0;
  if(host) {
    p = strchr(host, ':');
    if(p) {
      *p = '\0';
      su->port = atoi(p+1);
    }
    if(strlen(host)) {
      su->host = strdup(host);
      if(su->host==0) err |= 1;
    }
  }

  /* extract user name and password */
  if(login) {
    p = strchr(login, ':');
    if(p) {
      *p = '\0';
      su->password = strdup(p+1);
      if(su->password==0) err |= 1;
    }
    su->username = strdup(login);
    if(su->username==0) err |= 1;
  }

  /* done */
 done:
  if(err) lpgdv_protocol_free_url(su);
  if(url_copy) free(url_copy);
  LEAVE;
  return !err;
  
}

/*****************************************************************************/

void
lpgdv_protocol_free_url(LpgdvSplittedUrl* su)
{
  ENTER("lpgdv_protocol_free_url");
  if(su) {
    if(su->username)  free((void*)su->username);
    if(su->password)  free((void*)su->password);
    if(su->host)      free((void*)su->host);
    if(su->path)      free((void*)su->path);
    if(su->arguments) free((void*)su->arguments);
    memset(su, 0, sizeof(LpgdvSplittedUrl));
  }
  LEAVE;
}

/*****************************************************************************/
