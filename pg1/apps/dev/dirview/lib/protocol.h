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

   $Id$
*/

#ifndef __LIBPG_DIRVIEW_PROTOCOL_H__
#define __LIBPG_DIRVIEW_PROTOCOL_H__

#include "properties.h"


typedef void* (*LpgdvProtocolInitFunc)    (void);
typedef void  (*LpgdvProtocolUninitFunc)  (void* pdata);
typedef int   (*LpgdvSiteEnterFunc)       (void* pdata,
					   const char* site_name, int port,
					   const char* username,
					   const char* password);
typedef int   (*LpgdvSiteLeaveFunc)       (void* pdata,
					   const char* site_name, int port);

typedef int   (*LpgdvDirEnterFunc)        (void* pdata, const char* dir_name);
typedef int   (*LpgdvDirLeaveFunc)        (void* pdata, const char* dir_name);

typedef int   (*LpgdvItemNextFunc)        (void* pdata,
					   int row_nr, void** ref,
					   LpgdvProperties* prop);

typedef int   (*LpgdvItemLoadFunc)        (void* pdata,
					   const char* full_path,
					   const char* item_name,
					   const char* local_name,
					   int show_progress);
/* static (instance-less) functions*/
typedef const char*(*LpgdvGetdirnameFunc) (const char* path);
typedef const char*(*LpgdvGetitemnameFunc)(const char* path);
typedef int(*LpgdvCompareFunc)(const char*, const char*);

/*
 * The protocol descriptor
 */
typedef struct {
  const char* name;
  unsigned int property_id_set;  /* or-ed supported LpgdvStdPropertyId */
  int has_login:1;
  int has_host:1;
  int has_arguments:1;
  int default_port;
  LpgdvProtocolInitFunc    protocol_init_f;
  LpgdvProtocolUninitFunc  protocol_uninit_f;
  LpgdvSiteEnterFunc       site_enter_f;
  LpgdvSiteLeaveFunc       site_leave_f;
  LpgdvDirEnterFunc        dir_enter_f;
  LpgdvDirLeaveFunc        dir_leave_f;
  LpgdvItemNextFunc        item_next_f;
  LpgdvItemLoadFunc        item_load_f;
  LpgdvGetdirnameFunc      getdirname_f;
  LpgdvGetitemnameFunc     getitemname_f;
  LpgdvCompareFunc         is_same_site_f;
  LpgdvCompareFunc         is_same_dir_f;
} LpgdvProtocol;


/*
 * The protocols table
 */
extern const LpgdvProtocol* protocols[];

/**
 * Determines the protocol of the given url
 *
 * @param name
 *   [IN] the protocol name, e.g. "file" or "ftp"
 *
 * @returns
 *   The protocol descriptor, or zero;
 */
extern const LpgdvProtocol* lpgdv_protocol_find(const char* name);

typedef struct {
  const char* username;
  const char* password;
  const char* host;
  int port;
  const char* path;
  const char* arguments;
} LpgdvSplittedUrl;

typedef struct {
  const char* dir;
  const char* item;
} LpgdvSplittedPath;

extern int lpgdv_protocol_split_url(const char* url,
				    const LpgdvProtocol* prot,
				    LpgdvSplittedUrl* su);
extern void lpgdv_protocol_free_url(LpgdvSplittedUrl* su);




#endif /* __LIBPG_DIRVIEW_PROTOCOL_H__ */
