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

#include "wclock.h"
#include "map.inc"

void
get_world_size(int* w, int* h)
{
  *w = dims[0];
  *h = dims[1];
}

int
get_world_width(void)
{
  return dims[0];
}

int
get_world_height(void)
{
  return dims[1];
}

pghandle
create_world_bitmap()
{
  ENTER("create_world_bitmap()");
  pghandle ret = pgNewBitmap(pgFromMemory(map, sizeof(map)));
  LEAVE;
  return ret;
}

/*****************************************************************************/
