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

#ifndef __LIBPG_DIRVIEW_PROPERTIES_H__
#define __LIBPG_DIRVIEW_PROPERTIES_H__

#include <time.h>
#include "mime.h"
#include "displet.h"

/**
 * Standard properties
 */
typedef enum {
  LPGDV_PROP_TYPE         = 1<<0, /**< chr blk lnk p=fifo socket dir file */
  LPGDV_PROP_NAME         = 1<<1,
  LPGDV_PROP_PROTECTION   = 1<<2, /**< rwxrwxrwx */
  LPGDV_PROP_SIZE         = 1<<3,
  LPGDV_PROP_OWNER        = 1<<4,
  LPGDV_PROP_GROUP        = 1<<5,
  LPGDV_PROP_CRE_DATE     = 1<<6,
  LPGDV_PROP_MOD_DATE     = 1<<7,
  LPGDV_PROP_ACC_DATE     = 1<<8,
  LPGDV_PROP_MIME_TYPE    = 1<<9,
} LpgdvStdPropertyId;

/**
 * A displet can create custom properties. The IDs are customly defined
 * and bits 24-31 are the displet class. This allows 24 properties for
 * each of 256 displet classes.
 */
static inline unsigned int
properties_make_id(LpgdvDispletClass class, unsigned int id)
{
  return (id & 0xffffff) | (id<<24);
}

/* For now we have one struct with all possible properties (even those that
 * are irrelevant for a given protocol.
 *
 * We may rather want to make this struct an union, and have one array
 * being a list of N LpgdvPropertyId for the protocol, and N properties
 * per item in the corresponding order. This would make less data per
 * item.
 */

typedef struct {
  char type;
  const char* name;
  const char* display_name; /* if null, use name */
  const char* protections;
  unsigned long long size;
  unsigned long owner;
  unsigned long group;
  time_t cre_date;
  time_t mod_date;
  time_t acc_date;
  LpgdvMimeTypeId mime_type;
  void* custom_1;
  void* custom_2;
} LpgdvProperties;

#endif /* __LIBPG_DIRVIEW_PROPERTIES_H__ */
