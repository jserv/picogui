/* $Id$
 *
 * libpg_dirview - A directory browser based on the source code of cli_c's
 *                 dlg_filepicker.c
 *
 * This code is oozing with Unixisms, so it will need to be ported to run
 * on other OSes... 
 *
 * - The directory separator is assumed to be '/'
 * - The user's home directory is retrieved through $HOME
 * - Multiple drives (as in DOS) are not supported
 * - Unix functions are used to implement the file tools and validate
 *   files for the PG_FILE_MUST* flags
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
 *  o Dirview component architecture and design:
 *    Copyright (C) 2002 Pascal Bauermeister <pascal.bauermeister@urbanet.ch>
 * 
 * 
 * 
 */

#include "libpg_dirview.h"
#include "mime.h"
#include "protocol.h"

#include <string.h>     /* strcpy(), strcat() */
#include <stdlib.h>     /* For getenv() */
#include <sys/types.h>  /* opendir(), readdir(), etc */
#include <dirent.h>
#include <sys/stat.h>   /* stat() */
#include <unistd.h>
#include <errno.h>      /* Check errors when validating a filename */
#include <ctype.h>

pghandle wApp;

int main(int argc, char** argv)
{
  const char* newfile;
  pgInit(argc,argv);
  wApp = pgRegisterApp(PG_APP_NORMAL, "dirview", 0);

  if(0) newfile = _dirview(NULL, NULL, NULL, 0*PG_FILEOPEN,
			  "Load a file");
  else dirview("Test",
	       //"file://.",
	       //"file://pascal:lombric@localhost/a/b/c/toto.html?123",
	       //"file://localhost/a/b/c/toto.html?123",
	       //"file:/a/b/c/toto.html",
	       //"file:///a/b/c/toto.html?abcd",
	       "file:///usr/local/",
	       //"http:/a/b/c/toto.html?123",
	       //"http:/a/b/c/toto.html",
	       LPGDV_TITLE,                       "Yellow world",
	       LPGDV_TITLE_VISIBLE,               MENU,
	       
	       LPGDV_MENU_TITLE,                  "My Menu",
	       LPGDV_MENU_RENDERING,              MENU,
	       
	       LPGDV_MENU_ITEM_ADD,               "Menu item 1",
	       LPGDV_MENU_ITEM_ENABLE_FUNC,       0x123456,
	       LPGDV_MENU_ITEMS_ENABLE_FUNC,      0x7891bc,
	       LPGDV_MENU_ITEM_ADD,               "Menu item 2",
	       LPGDV_MENU_ITEM_ENABLE_FUNC,       0x123456,
	       LPGDV_MENU_ITEMS_ENABLE_FUNC,      0x7891bc,
	       
	       LPGDV_LOCATION_VISIBLE,            EDITABLE,
	       
	       LPGDV_LIST_HEADER_VISIBLE,         TOGGLESORT,
	       LPGDV_LIST_SORT_BY_COL_NR,         2,
	       LPGDV_LIST_INCLUDE_PARENT,         NO, //YES,
	       LPGDV_LIST_SORT,                   RSORT,
	       LPGDV_LIST_GROUP,                  DIRS_FIRST,
	       
	       LPGDV_BROWSE_SITE_ENTER_FUNC,      0xabcd,
	       LPGDV_BROWSE_SITE_LEAVE_FUNC,      0xdef0,
	       
	       LPGDV_BROWSE_DIR_ENTER_FUNC,       0xba6dad,
	       LPGDV_BROWSE_DIR_LEAVE_FUNC,       0xcafee,
	  
	       LPGDV_BROWSE_ITEM_NEXT_FUNC,       0xbabe,
	       
	       LPGDV_COLUMN_ADD,                  "Col 1",
	       LPGDV_COLUMN_PLACE_AT,             AT_RIGHT,
	       LPGDV_COLUMN_RENDER_FUNC,          0xdead,
	       LPGDV_COLUMN_RENDER_PAYLOAD,       10,
	       LPGDV_COLUMN_ADD,                  "Col 2",
	       LPGDV_COLUMN_PLACE_AT,             AT_LEFT,
	       LPGDV_COLUMN_RENDER_PAYLOAD,       20,
	       LPGDV_COLUMN_ADD,                  "Col 3",
	       LPGDV_COLUMN_PLACE_AT,             AT_LEFT,
	       LPGDV_COLUMN_RENDER_PAYLOAD,       30,
	       
	       LPGDV_SELECTION_CHANGE_FUNC,       0xbeef,
	       
	       LPGDV_ITEM_FOCUS_FUNC,             0xf00,
	       LPGDV_ITEM_CLICK_FUNC,             0xbaa,
	       0, 0
	       );

  if(newfile) printf("=> [%s]\n", newfile);
  else        printf("=> <null>\n");

  {
    const char* fname = "xR-EADME-toto.anim9.xcf.bz2";
    const char* mname;
    LpgdvMimeTypeId id = lpgdv_mime_type_id_of(fname, -1);
    mname = lpgdv_mime_type_name_of(id);
    printf("object=[%s] id=%d mime-type=[%s]\n", fname, id, mname);
  }
  {
    const char* url = "file:///toto";
    const LpgdvProtocol* prot = lpgdv_protocol_find(url);
    printf("url=[%s] prot=%s\n", url, prot ? prot->name : "?");
  }

  return 0;
}

/* The End */
