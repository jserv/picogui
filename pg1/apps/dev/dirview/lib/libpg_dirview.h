/* $Id$
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * 
 * 
 */

#ifndef __LIBPG_DIRVIEW_H__
#define __LIBPG_DIRVIEW_H__

#include <picogui.h>

const char *_dirview(pgfilter filefilter, const char *pattern,
		     const char *deffile, int flags, const char *title);

const char *dirview(const char *view_name,
                    const char *url,
		    ...);

typedef struct {
  void* user_data;
} LpgdvInstance;

typedef enum {
  LPGDV_OK=0,
  LPGDV_ERROR_MEMORY,
  LPGDV_ERROR_INVALID_URL,
  LPGDV_ERROR_UNKNOWN_PROTOCOL,
} LpgdvStatus;


typedef int(*LpgdvMenuItemEnableF)  (LpgdvInstance*, int row_nr);
typedef int(*LpgdvMenuItemsEnableF) (LpgdvInstance*, int nb_rows, int* rows);

typedef int(*LpgdvBrowseSiteEnterF) (LpgdvInstance*, const char* site_name);
typedef int(*LpgdvBrowseSiteLeaveF) (LpgdvInstance*);

typedef int(*LpgdvBrowseDirEnterF)  (LpgdvInstance*, const char* site_name);
typedef int(*LpgdvBrowseDirLeaveF)  (LpgdvInstance*);

typedef int(*LpgdvBrowseItemNextF)  (LpgdvInstance*, int row_nr, void** ref);

typedef int(*LpgdvColumnRenderF)    (LpgdvInstance*, int row_nr, int col_nr,
				     void* payload, void* ref);

typedef int(*LpgdvSelectionChangedF)(LpgdvInstance*, int nb_items, int* items);

typedef int(*LpgdvItemFocusedF)(LpgdvInstance*, int row_nr);
typedef int(*LpgdvItemClickedF)(LpgdvInstance*, int row_nr);

typedef enum {
  NO = 0,
  YES = 1,
  MENU,
  BUTTONS,
  EDITABLE,
  SORT, RSORT, TOGGLESORT,
  DIRS_FIRST, DIRS_LAST,
  AT_LEFT, AT_RIGHT,
  AT_ABS, AT_REL,
} LpgdvVal;


typedef enum {
  /*
   * GLOBAL VISUAL
   */
  LPGDV_TITLE=1000,                  /* <str> */
  LPGDV_TITLE_VISIBLE,               /* NO / YES / MENU */
  LPGDV_LOCATION_VISIBLE,            /* YES / NO / EDITABLE */

  LPGDV_LIST_HEADER_VISIBLE=1100,    /* YES / NO / SORT / RSORT / TOGGLESORT */
  LPGDV_LIST_SORT_BY_COL_NR,         /* <num> */
  LPGDV_LIST_INCLUDE_PARENT,         /* YES / NO */
  LPGDV_LIST_SORT,                   /* NO / SORT / RSORT */
  LPGDV_LIST_GROUP,                  /* NO / DIRS_FIRST / DIRS_LAST */

  /*
   * DATATYPE VISUAL
   */
  LPGDV_COLUMN_ADD=2000,             /* <str> */
  LPGDV_COLUMN_PLACE,                /* AT_LEFT / AT_RIGHT / AT_ABS / AT_REL */
  LPGDV_COLUMN_PLACE_AT,             /* <num> */
  LPGDV_COLUMN_RENDER_FUNC,          /* LpgdvColumnRenderF */
  LPGDV_COLUMN_RENDER_PAYLOAD,       /* void* */

  /*
   * USER INTERACTION
   */
  LPGDV_SELECTION_CHANGE_FUNC=3000,  /* LpgdvSelectionChangedF */

  LPGDV_ITEM_FOCUS_FUNC=3100,        /* LpgdvItemFocusedF */
  LPGDV_ITEM_CLICK_FUNC,             /* LpgdvItemClickedF */

  /*
   * CONTEXTUAL MENU
   */
  LPGDV_MENU_TITLE=4000,             /* <str> */
  LPGDV_MENU_RENDERING,              /* NO / MENU | BUTTONS */

  LPGDV_MENU_ITEM_ADD=4100,          /* <str> */
  LPGDV_MENU_ITEM_ENABLE_FUNC,       /* LpgdvMenuItemEnableF */
  LPGDV_MENU_ITEMS_ENABLE_FUNC,      /* LpgdvMenuItemsEnableF */

  /*
   * PROTOCOL
   */
  LPGDV_PROTOCOL_DEFAULT=5000,       /* <str> */

  LPGDV_BROWSE_SITE_ENTER_FUNC=5100, /* LpgdvBrowseSiteEnterF */
  LPGDV_BROWSE_SITE_LEAVE_FUNC,      /* LpgdvBrowseSiteLeaveF */

  LPGDV_BROWSE_DIR_ENTER_FUNC=5200,  /* LpgdvBrowseDirEnterF */
  LPGDV_BROWSE_DIR_LEAVE_FUNC,       /* LpgdvBrowseDirLeaveF */

  LPGDV_BROWSE_ITEM_NEXT_FUNC=5300,  /* LpgdvBrowseItemNextF */
} LpgdvTag;



#endif /* __LIBPG_DIRVIEW_H__ */
