/*
   Copyright (C) 2002 by Pascal Bauermeister

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with PocketBee; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Pascal Bauermeister
   Contributors:

   $Id: proto_file.c,v 1.1 2002/07/08 04:29:43 bauermeister Exp $
*/

#include <string.h>
#include "libpg_dirview.h"
#include "protocol.h"
#include "debug.h"


static void* file_protocol_init(void);
static void  file_protocol_uninit(void*);

static int file_site_enter(void*, const char* site_name, int port,
			   const char* username, const char* password);
static int file_site_leave(void*, const char* site_name, int port);

static int file_dir_enter(void*, const char* dir_name);
static int file_dir_leave(void*, const char* dir_name);

static int file_item_next(void*,
			  int row_nr, void** ref,
			  LpgdvProperties* prop);
static int file_item_load(void* pdata,
			  const char* full_path,
			  const char* item_name,
			  const char* local_name,
			  int show_progress);

static const char* file_getdirname(const char* path);
static const char* file_getitemname(const char* path);

static int file_is_same_site(const char*, const char*);
static int file_is_same_dir(const char*, const char*);

const LpgdvProtocol file_protocol = {
  "file",
  ( LPGDV_PROP_TYPE |
    LPGDV_PROP_NAME |
    LPGDV_PROP_PROTECTION |
    LPGDV_PROP_SIZE |
    LPGDV_PROP_OWNER |
    LPGDV_PROP_GROUP |
    LPGDV_PROP_CRE_DATE |
    LPGDV_PROP_MOD_DATE |
    LPGDV_PROP_ACC_DATE |
    LPGDV_PROP_MIME_TYPE
    ),
  0, /* has_login */
  0, /* has_host */
  1, /* has_arguments */
  0, /* default_port */
  file_protocol_init,
  file_protocol_uninit,
  file_site_enter,
  file_site_leave,
  file_dir_enter,
  file_dir_leave,
  file_item_next,
  file_item_load,
  file_getdirname,
  file_getitemname,
  file_is_same_site,
  file_is_same_dir,
};

static void*
file_protocol_init(void)
{
  ENTER("file_protocol_init()");
  LEAVE;
  return (void*)1;
}

static void
file_protocol_uninit(void* pdata)
{
  ENTER("file_protocol_uninit()");
  LEAVE;
  return;
}

static int
file_site_enter(void* pdata, const char* site_name, int port,
		const char* username, const char* password)
{
  ENTER("file_site_enter()");
  LEAVE;
  return 1;
}

static int
file_site_leave(void* pdata, const char* site_name, int port)
{
  ENTER("file_site_leave()");
  LEAVE;
  return ;
}

static int
file_dir_enter(void* pdata, const char* site_name)
{
  ENTER("file_dir_enter()");
  LEAVE;
  return 1;
}

static int
file_dir_leave(void* pdata, const char* site_name)
{
  ENTER("file_dir_leave()");
  LEAVE;
  return 1;
}

static int
file_item_next(void* pdata, int row_nr, void** ref, LpgdvProperties* prop)
{
  ENTER("file_item_next()");
  LEAVE;
  return 0;
}

static int
file_item_load(void* pdata,
	       const char* full_path,
	       const char* item_name,
	       const char* local_name,
	       int show_progress)
{
  return -1;
}

static const char*
file_getdirname(const char* path)
{
  ENTER("file_getdir()");
  char* dir = 0;
  if(path && strlen(path) && (dir=strdup(path)) ) {
    char* p = strrchr(dir, '/'); /* unix ! */
    if(p) *++p = '\0';
  }
  LEAVE;
  return dir;
}

static const char*
file_getitemname(const char* path)
{
  ENTER("file_getdir()");
  char* file = 0;
  if(path && strlen(path)) {
    char* p = strrchr(path, '/'); /* unix ! */
    if(p) file = strdup(p+1);
  }
  LEAVE;
  return file;
}

static int
file_is_same_site(const char* name1, const char* name2)
{
  /* site not supported: always match ! */
  return 1;
}

static int
file_is_same_dir(const char* name1, const char* name2)
{
  if(name1==0 && name2==0) return 0;
  if(name1==0) return 0;
  if(name2==0) return 0;
  return !strcmp(name1, name2);
}
