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

#ifndef __LIBPG_DIRVIEW_DISPLET_H__
#define __LIBPG_DIRVIEW_DISPLET_H__

typedef enum {
  LPGDV_DISP_STD = 0,  /**< standard properties */
  LPGDV_DISP_FILE,
} LpgdvDispletClass;

typedef enum {
  LPGDV_DISP_FILE_LIST_SHOWDEV      =  1<<0,
  LPGDV_DISP_FILE_LIST_SHOWDOT      =  1<<1,
  LPGDV_DISP_FILE_LIST_SHOWHIDDEN   =  1<<2, 
  LPGDV_DISP_FILE_LIST_SHOWBAK      =  1<<3,
  LPGDV_DISP_FILE_LIST_DIRSFIRST    =  1<<4,
  LPGDV_DISP_FILE_LIST_DIRSLAST     =  1<<5,
  LPGDV_DISP_FILE_LIST_SORT         =  1<<6,
  LPGDV_DISP_FILE_LIST_REVERSESORT  =  1<<7,
} LpgdvFileListDisplay;


#endif /* __LIBPG_DIRVIEW_DISPLET_H__ */
