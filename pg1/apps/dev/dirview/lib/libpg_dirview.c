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

#include <string.h>     /* strcpy(), strcat() */
#include <stdlib.h>     /* For getenv() */
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>      /* Check errors when validating a filename */
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>   /* stat() */
#include <sys/types.h>  /* opendir(), readdir(), etc */


#include "libpg_dirview.h"
#include "properties.h"
#include "protocol.h"

#define DECLARE_DEBUG_VARS
#include "debug.h"

/*****************************************************************************/

typedef struct {
  const char*            name;
  LpgdvMenuItemEnableF   menu_item_enable;
  LpgdvMenuItemsEnableF  menu_items_enable;
} MenuItemParams;


typedef struct {
  const char*            name;
  LpgdvVal               place;
  int                    place_at;
  LpgdvColumnRenderF     column_render;
  void*                  payload;
} ColumnParams;


typedef struct {
  /* Params passed in function call */
  const char*            title;
  LpgdvVal               title_is_visible;
  LpgdvVal               location_visible;

  LpgdvVal               list_is_header_visible;
  int                    list_sort_by_col_nr;
  LpgdvVal               list_do_include_parent;
  LpgdvVal               list_sort;
  LpgdvVal               list_group;

  LpgdvSelectionChangedF selection_changed;
  LpgdvItemFocusedF      item_focused;
  LpgdvItemClickedF      item_clicked;

  const char*            menu_title;
  LpgdvVal               menu_rendering;

  const char*            protocol_default;
  LpgdvBrowseSiteEnterF  browse_site_enter;
  LpgdvBrowseSiteLeaveF  browse_site_leave;
  LpgdvBrowseDirEnterF   browse_dir_enter;
  LpgdvBrowseDirLeaveF   browse_dir_leave;
  LpgdvBrowseItemNextF   browse_item_next;

  /* Calculated params */
  int                    menu_items_nb;
  int                    columns_nb;
} Parameters;

/*****************************************************************************/

/* This is the size of the buffer files are returned in */
#define FILEMAX 512

/* This is the maximum size of an individual file name */
#define NAMEMAX 80

/* All items in the file list box have this as their payload
 * value, so we can easily see if a widget is a file item.
 *
 * Files will be the only widgets with payloads in this dialog,
 * but the danger here is of outside widgets. In the event that
 * the file dialog is called from a toolbar app, the toolbar could
 * send events while the file picker is open. Normally toolbar widgets
 * will not have payloads, but if one has this payload it could really
 * confuse the file picker.
 */
#define FILETAG   0x19840713

/* Save the current directory across instances. */
char dirview_dir[FILEMAX];

/* Static buffer for returning the file name */
char dirview_buf[FILEMAX];

/* A structure for passing data to dirview_setdir */
struct filepickdata {
  /* Important containers */
  pghandle wDirectory;
  pghandle wFileList;
  pghandle wScroll;

  /* Filter parameters */
  int flags;
  pgfilter filefilter;
  const char *pattern;
  
  /* Fonts */
  pghandle fDirectory, fLink, fHeading;

  /* Selected file */
  pghandle sFileName, wFile;
};

/* This is the type of node used to hold a directory entry */
struct filenode {
  char name[NAMEMAX];
  struct stat st;
};

static int dirview_filter(struct filepickdata *dat, const char *name,
			  struct stat *st);
static void dirview_fullpath(const char *file);
static void dirview_setdir(struct filepickdata *dat);
static int  dirview_compare(const void *a, const void *b);
static void dirview_pathmenu(struct filepickdata *dat);


/*****************************************************************************/

static int __indent = 0;
#define INDENT        { ++__indent; }
#define UNINDENT      { --__indent; }

#define _PR(x...)     fprintf(stderr, x)
#define _I            { int i; for(i=0;i<__indent;++i) _PR("  "); }
#define _DOTS         ". . . . . . . . . . . . . . . . . . . ."
#define _OUTM(m,f,v)  { _PR(_DOTS " " f "\r", v); _I; _PR(m " \n"); }

#define DUMP(f,v)     { _I; _PR(f, v); }
#define DUMP_STR(s)   _OUTM(#s, "[%s]", p->s ? p->s : "<null>");
#define DUMP_NUM(s)   _OUTM(#s, "%d", p->s);
#define DUMP_FUNC(s)  _OUTM(#s, "0x%08x", p->s);
#define DUMP_VAL(s)   _OUTM(#s, "%s", ValTxt[p->s]);

static const char* ValTxt[] = {
  "NO",
  "YES",
  "MENU",
  "BUTTONS",
  "EDITABLE",
  "SORT",
  "RSORT",
  "TOGGLESORT",
  "DIRS_FIRST",
  "DIRS_LAST",
  "AT_LEFT",
  "AT_RIGHT",
  "AT_ABS",
  "AT_REL",
};

static void
dump_menuitemparams(int nb, MenuItemParams* p)
{
  int i;
  DUMP("MenuItemParams:\n", 0);
  INDENT;
  for (i=0; i<nb; ++i) {
    DUMP      ("[%d]\n", i);
    INDENT;
    DUMP_STR  (name);
    DUMP_FUNC (menu_item_enable);
    DUMP_FUNC (menu_items_enable);
    UNINDENT;
    ++p;
  }
  UNINDENT;
}

dump_columnparams(int nb, ColumnParams* p)
{
  int i;
  DUMP("ColumnParams:\n", 0);
  INDENT;
  for (i=0; i<nb; ++i) {
    DUMP      ("[%d]\n", i);
    INDENT;
    DUMP_STR  (name);
    DUMP_VAL  (place_at);
    DUMP_FUNC (column_render);
    DUMP_NUM  (payload);
    UNINDENT;
    ++p;
  }
  UNINDENT;
}

static void
dump_params(Parameters *p)
{
  DUMP("Params:\n", 0);
  INDENT;
  DUMP_STR  (title);
  DUMP_VAL  (title_is_visible);
  DUMP_VAL  (location_visible);

  DUMP_VAL  (list_is_header_visible);
  DUMP_NUM  (list_sort_by_col_nr);
  DUMP_VAL  (list_do_include_parent);
  DUMP_VAL  (list_sort);
  DUMP_VAL  (list_group);

  DUMP_FUNC (selection_changed);

  DUMP_FUNC (item_focused);
  DUMP_FUNC (item_clicked);

  DUMP_STR  (menu_title);
  DUMP_VAL  (menu_rendering);

  DUMP_FUNC (protocol_default);
  DUMP_FUNC (browse_site_enter);
  DUMP_FUNC (browse_site_leave);
  DUMP_FUNC (browse_dir_enter);
  DUMP_FUNC (browse_dir_leave);
  DUMP_FUNC (browse_item_next);

  DUMP_NUM  (menu_items_nb);
  DUMP_NUM  (columns_nb);
  UNINDENT;
}

/*****************************************************************************/
/* This is just like pgMenuFromString, except that we use
 * '/' as the separator and the items are listed backwards,
 * ending with a bare '/'. When the user chooses a path, 
 * set the dialog's directory
 */

#if 0
static void
dirview_pathmenu(struct filepickdata *dat)
{
  char *p;
  char *items = dirview_dir;
  pghandle str;
  int ret;
  int i;
  struct stat st;
  
  /* Create the menu popup in its own context */
  pgEnterContext();
  pgNewPopupAt(PG_POPUP_ATCURSOR,PG_POPUP_ATCURSOR,0,0);

  /* Add helpful tools! */

  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_TEXT,pgNewString("Rename"),
	      0);
  pgSetPayload(PGDEFAULT,-4);

  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_TEXT,pgNewString("Delete"),
	      0);
  pgSetPayload(PGDEFAULT,-3);

  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_TEXT,pgNewString("New Directory"),
	      0);
  pgSetPayload(PGDEFAULT,-2);

  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_TEXT,pgNewString("Home"),
	      0);
  pgSetPayload(PGDEFAULT,-1);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_TEXT,pgNewString("Tools:"),
	      0);
  pgSetPayload(PGDEFAULT,-4);

  /* Add directories above this one */
  i=0;
  do {
    /* Do a little fancy stuff to make the string handle.
     * This is like pgNewString but we get to specify the 
     * length instead of having strlen() do it for us.
     */
    if (!(p = strchr(items,'/'))) p = items + strlen(items);
    if (p==items)
      str = pgNewString("/");   /* Root directory */
    else {
      _pg_add_request(PGREQ_MKSTRING,(void *) items,p-items);
      pgFlushRequests();
      str = _pg_return.e.retdata;      
    }
    items = p+1;

    /* Ignore the last part, because that's the current directory */
    if (*p) {

      /* Create each menu item */
      pgNewWidget(PG_WIDGET_MENUITEM,0,0);
      pgSetWidget(PGDEFAULT,
		  PG_WP_TEXT,str,
		  PG_WP_SIDE,PG_S_BOTTOM,
		  0);
      
      /* Payload is the index in dirview_dir to set to zero
       * if it is chosen */
      i = items-dirview_dir;
      if (i>1)
	i--;
      pgSetPayload(PGDEFAULT,i);
    }
    
  } while (*p);

  /* Run the menu */
  ret = pgGetPayload(pgGetEvent()->from);
  pgLeaveContext();
  if (ret>0) {
    dirview_dir[ret] = 0;
    dirview_setdir(dat);
  }
  else if (ret<0) {
    /* Update selected file string */
    if (dat->wFile)
      dat->sFileName = pgGetWidget(dat->wFile,PG_WP_TEXT);
    
    switch (-ret) {

    case 1 :    /* Home */
      dirview_dir[FILEMAX-1] = 0;
      strncpy(dirview_dir,getenv("HOME"),FILEMAX-1);
      dirview_setdir(dat);
      break;
      
    case 2:     /* New Directory */
      str = pgInputDialog("New Directory", "Name:",0);
      if (str) {
	dirview_fullpath(pgGetString(str));
	pgDelete(str);
	
	if (mkdir(dirview_buf,0777)) 
	  pgMessageDialogFmt("Error",0,"Unable to create directory:\n%s",
			     dirview_buf);
	else
	  dirview_setdir(dat);
    }
      break;
      
    case 3:     /* Delete */
      if (dat->sFileName) {
	dirview_fullpath(pgGetString(dat->sFileName));
	if (pgMessageDialog("Delete?",dirview_buf,
			    PG_MSGBTN_YES | PG_MSGBTN_NO) == PG_MSGBTN_YES) {
	  
	  if (unlink(dirview_buf))
	    pgMessageDialogFmt("Error",0,"Unable to delete file:\n%s",
			       dirview_buf);
	  else
	    dirview_setdir(dat);
	}
      }
      break;
      
    case 4:     /* Rename */
      if (dat->sFileName) {
	dirview_fullpath(pgGetString(dat->sFileName));
	str = pgInputDialog("Rename File",dirview_buf,0);
	if (str) {
	  char *oldname, *newname;
	  oldname = strdup(dirview_buf);
	  newname = pgGetString(str);
	  if (newname[0] != '/') {
	    dirview_fullpath(newname);
	    newname = dirview_buf;
	  }
	  
	  if (rename(oldname,newname))
	    pgMessageDialogFmt("Error",0,"Unable to rename file:\n%s\nto\n%s",
			       oldname,newname);
	  else
	    dirview_setdir(dat);
	  
	  free(oldname);
	}
      }
      break;
            
    }
  }
}

#endif

/*****************************************************************************/
/* Sort the files, in a way that should make sense to users.
 * Directories always come before files. Case is ignored, punctuation
 * is ignored. If the file has a trailing number, the numbers are sorted
 * numerically.
 */

static int
dirview_compare(const void *a, const void *b)
{
  struct filenode *f1 = (struct filenode *) a;
  struct filenode *f2 = (struct filenode *) b;
  const char *n1 = f1->name;
  const char *n2 = f2->name;
  int d1 = S_ISDIR(f1->st.st_mode);
  int d2 = S_ISDIR(f2->st.st_mode);

  /* If one is a directory and one is not, the directory comes first. */
  if (d1!=d2) {
    if (d1)
      return -1;
    else
      return 1;
  }

  /* Now sort them pseudo-alphabetically */
  while (*n1 && *n2) {

    /* Skip punctuation */
    if (!isalnum(*n1)) {
      n1++;
      continue;
    }
    if (!isalnum(*n2)) {
      n2++;
      continue;
    }

    if (isdigit(*n1) && isdigit(*n2)) {
      /* If they are both numbers, sort them numerically */
      
      char *p;
      int u1,u2;
      u1 = strtoul(n1,&p,10);
      n1 = p-1;
      u2 = strtoul(n2,&p,10);
      n2 = p-1;
      if (u1<u2)
	return -1;
      else if (u1>u2)
	return 1;
    }
    else {
      /* Nope, do a case-insensitive asciibetical sort */

      char c1,c2;
      c1 = tolower(*n1);
      c2 = tolower(*n2);
      if (c1<c2)
	return -1;
      else if (c1>c2)
	return 1;
    }

    /* The same so far, so advance both */
    n1++;
    n2++;
  }

  /* Compare length */
  if (n1)
    return -1;
  if (n2)
    return 1;
  return 0;
}

/*****************************************************************************/
/* Utility that, given the current filters and flags, determines if a file
 * should be visible. Returns nonzero if the file should be visible.
 */

static int
dirview_filter(struct filepickdata *dat, const char *name,
	       struct stat *st)
{ 
  /* Normally ignore device nodes. Listing the contents of /dev could
   * be disasterous, especially on a low-memory machine
   */
  if ((S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode)) 
      && !(dat->flags & PG_FILE_SHOWDEV))
    return 0;

  /* Normally we should hide files beginning with a dot */
  if (*name=='.') {
    /* Is it '.' or '..' ? */
    if (name[1]=='.' || !name[1]) {
      if (!(dat->flags & PG_FILE_SHOWDOT))
	return 0;
    }
    else {
      if (!(dat->flags & PG_FILE_SHOWHIDDEN))
	return 0;
    }
  }

  /* Always include directories if they pass the dot-test above */
  if (S_ISDIR(st->st_mode))
    return 1;

  /* Normally skip editor backups */
  if (!(dat->flags & PG_FILE_SHOWBAK)) {
    char first, last;

    first = *name;
    last  = name[strlen(name)-1];

    if (last == '~')
      return 0;
    if (first == '#' && last == '#')
      return 0;
  }

  /* If there is no filter, just include the file */
  if (!dat->filefilter)
    return 1;
      
  /* The rest depends on the user-defined filter */
  return (*dat->filefilter)(name,dat->pattern);
}

/*****************************************************************************/
/* Given a filename, constructs it's full path in dirview_buf
 * careful to avoid overflowing any buffers. 
 */

static void
dirview_fullpath(const char *file)
{
  int len = strlen(dirview_dir);
  strcpy(dirview_buf,dirview_dir);
  if (len<(FILEMAX-1) && dirview_buf[len-1]!='/') {
    strcat(dirview_buf,"/");
    len--;
  }
  strncat(dirview_buf,file,FILEMAX-1-len);
}

/*****************************************************************************/
/* Utility to populate the dialog box with the current directory's files */

static void
dirview_setdir(struct filepickdata *dat)
{
  DIR *d;
  struct dirent *arthur;     /* Dirent, Arthur Dirent... */
  struct stat st;
  struct filenode *names, *p;
  int total = 0, count, i;
  int itemheight;
  char *s;
  pghandle font;
  pghandle wNameBoxP, wSizeBoxP, wXyzBoxP;
  char buf[20];

  /* Clear the directory context */
  pgLeaveContext();
  pgEnterContext();

  /* Scroll back to the top */
  pgSetWidget(dat->wScroll,
	      PG_WP_VALUE,0,
	      0);
  
  /* Set the directory button's text. We don't need to use replacetext here
   * because clearing the context also takes care of this string handle.
   *
   * Use strrchr() to display only the last directory name in the path
   */
  s = strrchr(dirview_dir,'/');
  if (s[1]) s++;
  pgSetWidget(dat->wDirectory,PG_WP_TEXT,pgNewString(s),0);

  /* Select the relevant entries, store them, and sort them. This is just
   * like what scandir() does, but scandir doesn't allow passing extra
   * data to the selection function. I suppose another reason to avoid
   * scandir() is that uClibc doesn't yet implement it correctly...
   */

  /* first just count the files... */
  d = opendir(dirview_dir);
  if (!d) return;
  while (readdir(d))
    total++;
  rewinddir(d);

  /* ... so we can allocate the array */
  names = malloc(sizeof(struct filenode) * total);
  if (!names) {
    closedir(d);
    return;
  }

  /* Now copy all the relevant directory entries */
  count = 0;
  p = names;
  while (count<total && (arthur = readdir(d))) {
    dirview_fullpath(arthur->d_name);
    lstat(dirview_buf,&st);
    
    if (dirview_filter(dat,arthur->d_name,&st)) {
    
      /* We want this file, so store it */
      memcpy(&p->st,&st,sizeof(st));
      p->name[NAMEMAX-1] = 0;
      strncpy(p->name,arthur->d_name,NAMEMAX-1);

      p++;
      count++;
    }
  }
  closedir(d);

  if (!count) {
    /* No items? It's like a "this page intentionally left blank" message */
    pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,dat->wFileList);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE,PG_S_ALL,
		PG_WP_TEXT,pgNewString("(no visible files)"),
		0);
  }
  else {
    /* Normal item drawing stuff... */

    /* Get the height for list items */
    itemheight = pgThemeLookup(PGTH_O_LISTITEM,PGTH_P_HEIGHT);
 
    /* Sort them */
    qsort(names,count,sizeof(struct filenode),&dirview_compare);
    
    /* Make Columns */
    wNameBoxP = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,dat->wFileList);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,1,
		PG_WP_SIDE,PG_S_LEFT,
		0);
    wSizeBoxP = pgNewWidget(PG_WIDGET_BOX,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,1,
		PG_WP_SIDE,PG_S_LEFT,
		0);
    wXyzBoxP = pgNewWidget(PG_WIDGET_BOX,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,1,
		PG_WP_SIDE,PG_S_LEFT,
		0);
    /* Column headings */
    wNameBoxP = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wNameBoxP);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,0,
		PG_WP_FONT,dat->fHeading,
		PG_WP_TEXT,pgNewString("Name"),
		PG_WP_ALIGN,PG_A_LEFT,
		PG_WP_SIZE,itemheight,
		0);
    
    wSizeBoxP = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wSizeBoxP);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,0,
		PG_WP_FONT,dat->fHeading,
		PG_WP_TEXT,pgNewString("Size"),
		PG_WP_ALIGN,PG_A_LEFT,
		PG_WP_SIZE,itemheight,
		0);

    wXyzBoxP = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wXyzBoxP);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,0,
		PG_WP_FONT,dat->fHeading,
		PG_WP_TEXT,pgNewString("Xyz"),
		PG_WP_ALIGN,PG_A_LEFT,
		PG_WP_SIZE,itemheight,
		0);
    
    /* List all the items */
    for (p=names,i=0;i<count;p++,i++) {
      
      /* Normally we'd use the default font, but directories and links
       * get special fonts
       */
      font = 0;
      if (S_ISLNK(p->st.st_mode))
	font = dat->fLink;
      /*
       * IMPORTANT: Directory must override link for directory symlinks
       *            to be followed. Currently the event loop checks the font
       *            to see if an item is a file or directory
       */
      if (S_ISDIR(p->st.st_mode))
	font = dat->fDirectory;
      
      /* Create the file name widget */
      wNameBoxP = pgNewWidget(PG_WIDGET_LISTITEM,PG_DERIVE_AFTER,wNameBoxP);
      pgSetWidget(PGDEFAULT,
		  PG_WP_TEXT,pgNewString(p->name),
		  PG_WP_ALIGN,PG_A_LEFT,
		  PG_WP_FONT,font,
		  0);
      pgSetPayload(PGDEFAULT,FILETAG);
      
      /* Listitems normally have PG_EXEV_TOGGLE and PG_EXEV_EXCLUSIVE turned
       * on. This makes them work basically like a radio button would.
       * They automatically turn off other listitems in the same container,
       * and they send an activate event when the mouse is pressed down.
       *
       * For directories, we don't care about hilighting (because we will redraw
       * it all anyway) and we want an activate when the mouse is released
       * after being pressed. In other words, a normal button.
       * If it's a directory, turn off all the EXEVs
       */
      if (font == dat->fDirectory)
	pgSetWidget(PGDEFAULT,
		    PG_WP_EXTDEVENTS,0,
		    0);
      
      /* FIXME: We'd like some icons here to indicate file type...
       * The icons themselves could be stored in a theme.
       * Determining file types might be more complex.
       */
      
      /* Make a more human-readable size */
 
      if (font)                                 /* Non-normal file? */
	strcpy(buf," -");
      else if (p->st.st_size > 1048576)         /* Megabytes? */
	sprintf(buf,"%d.%02d M",
		p->st.st_size / 1048576,
		(p->st.st_size / 10485) % 100);
      else if (p->st.st_size > 1024)            /* Kilobytes? */
	sprintf(buf,"%d.%02d K",
		p->st.st_size / 1024,
		(p->st.st_size / 10) % 100);
      else                                      /* Bytes? */
	sprintf(buf,"%d",p->st.st_size);
      
      /* Add another widget for size */
      wSizeBoxP = pgNewWidget(PG_WIDGET_LISTITEM,PG_DERIVE_AFTER,wSizeBoxP);
      pgSetWidget(PGDEFAULT,
		  PG_WP_TEXT,pgNewString(buf),
		  PG_WP_ALIGN,PG_A_LEFT,
		  PG_WP_SIZE,itemheight,
		  PG_WP_TRANSPARENT,0,
		  PG_WP_STATE,PGTH_O_LISTITEM,
		  0);

      /* Add another widget for Xyz */
      wXyzBoxP = pgNewWidget(PG_WIDGET_LISTITEM,PG_DERIVE_AFTER,wXyzBoxP);
      pgSetWidget(PGDEFAULT,
		  PG_WP_TEXT,pgNewString("Xyz"),
		  PG_WP_ALIGN,PG_A_LEFT,
		  PG_WP_SIZE,itemheight,
		  PG_WP_TRANSPARENT,0,
		  PG_WP_STATE,PGTH_O_LISTITEM,
		  //PG_WP_ON,1,
		  0);

    }
  }

  /* Free the memory! */
  free(names);
}

/*****************************************************************************/

static int va_count_tag(LpgdvTag tag, va_list va)
{
  int nb = 0;

  for (;;) {
    int t = va_arg(va, int);
    int v = va_arg(va, int);
    if(t==0 && v==0) break;
    if(t==tag) ++nb;
  }
  return nb;
}


Parameters params;


static LpgdvStatus
va_parse_params(Parameters* *params,
		MenuItemParams* *menu_item_params,
		ColumnParams* *column_params,
		va_list va)
{
  LpgdvStatus status = LPGDV_OK;
  int current_menu_item = -1;
  int current_column = -1;
  LpgdvColumnRenderF last_render_f = 0;

  *params = 0;
  *menu_item_params = 0;
  *column_params = 0;

  /* alloc mem for params */
  *params = malloc(sizeof(Parameters));
  if(*params == 0) {
    status = LPGDV_ERROR_MEMORY;
    goto done;
  }
  memset(*params, 0, sizeof(Parameters));

  /* count the number of menu items */
  (*params)->menu_items_nb = va_count_tag(LPGDV_MENU_ITEM_ADD, va);

  /* count the number of columns */
  (*params)->columns_nb = va_count_tag(LPGDV_COLUMN_ADD, va);

  /* alloc mem */
  if((*params)->menu_items_nb) {
    size_t size = (*params)->menu_items_nb * sizeof(MenuItemParams);
    *menu_item_params = malloc(size);
    if(*menu_item_params) memset(*menu_item_params, 0, size);
    else {
      status = LPGDV_ERROR_MEMORY;
      goto done;
    }
  }
  if((*params)->columns_nb) {
    size_t size = (*params)->columns_nb * sizeof(ColumnParams);
    *column_params = malloc(size);
    if(*column_params) memset(*column_params, 0, size);
    else {
      status = LPGDV_ERROR_MEMORY;
      goto done;
    }
  }

  /* parse tags */
  for (;;) {
    int tag = va_arg(va, int);
    void* val = va_arg(va, void*);

    if(tag==0 && val==0) break;
    switch(tag) {
    case LPGDV_TITLE:
      (*params)->title = (const char*)val;
      break;
    case LPGDV_TITLE_VISIBLE:
      (*params)->title_is_visible = (int)val;
      break;

    case LPGDV_LOCATION_VISIBLE:
      (*params)->location_visible = (int)val;
      break;
    case LPGDV_LIST_HEADER_VISIBLE:
      (*params)->list_is_header_visible = (int)val;
      break;
    case LPGDV_LIST_SORT_BY_COL_NR:
      (*params)->list_sort_by_col_nr = (int)val;
      break;
    case LPGDV_LIST_INCLUDE_PARENT:
      (*params)->list_do_include_parent = (int)val;
      break;
    case LPGDV_LIST_SORT:
      (*params)->list_sort = (int)val;
      break;
    case LPGDV_LIST_GROUP:
      (*params)->list_group = (int)val;
      break;

    case LPGDV_COLUMN_ADD:
      current_column++;
      (*column_params)[current_column].name = (const char*)val;
      (*column_params)[current_column].column_render = last_render_f;
      break;
    case LPGDV_COLUMN_PLACE:
      if(current_column<0) break;
      (*column_params)[current_column].place = (int)val;
      break;
    case LPGDV_COLUMN_PLACE_AT:
      if(current_column<0) break;
      (*column_params)[current_column].place_at = (int)val;
      break;
    case LPGDV_COLUMN_RENDER_FUNC:
      if(current_column<0) break;
      (*column_params)[current_column].column_render = (LpgdvColumnRenderF)val;
      last_render_f = (LpgdvColumnRenderF)val;
      break;
    case LPGDV_COLUMN_RENDER_PAYLOAD:
      if(current_column<0) break;
      (*column_params)[current_column].payload = (void*)val;
      break;

    case LPGDV_SELECTION_CHANGE_FUNC:
      (*params)->selection_changed = (LpgdvSelectionChangedF)val;
      break;

    case LPGDV_ITEM_FOCUS_FUNC:
      (*params)->item_focused = (LpgdvItemFocusedF)val;
      break;
    case LPGDV_ITEM_CLICK_FUNC:
      (*params)->item_clicked = (LpgdvItemClickedF)val;
      break;

    case LPGDV_MENU_TITLE:
      (*params)->menu_title = (const char*)val;
      break;
    case LPGDV_MENU_RENDERING:
      (*params)->menu_rendering = (int)val;
      break;

    case LPGDV_MENU_ITEM_ADD:
      current_menu_item++;
      (*menu_item_params)[current_menu_item].name = (const char*)val;
      break;
    case LPGDV_MENU_ITEM_ENABLE_FUNC:
      if(current_menu_item<0) break;
      (*menu_item_params)[current_menu_item]
	.menu_item_enable = (LpgdvMenuItemEnableF)val;
      break;
    case LPGDV_MENU_ITEMS_ENABLE_FUNC:
      if(current_menu_item<0) break;
      (*menu_item_params)
	[current_menu_item].menu_items_enable = (LpgdvMenuItemsEnableF)val;
      break;

    case LPGDV_PROTOCOL_DEFAULT:
      (*params)->protocol_default = (const char*)val;

    case LPGDV_BROWSE_SITE_ENTER_FUNC:
      (*params)->browse_site_enter = (LpgdvBrowseSiteEnterF)val;
      break;
    case LPGDV_BROWSE_SITE_LEAVE_FUNC:
      (*params)->browse_site_leave = (LpgdvBrowseSiteLeaveF)val;
      break;
    case LPGDV_BROWSE_DIR_ENTER_FUNC:
      (*params)->browse_dir_enter = (LpgdvBrowseDirEnterF)val;
      break;
    case LPGDV_BROWSE_DIR_LEAVE_FUNC:
      (*params)->browse_dir_leave = (LpgdvBrowseDirLeaveF)val;
      break;

    case LPGDV_BROWSE_ITEM_NEXT_FUNC:
      (*params)->browse_item_next = (LpgdvBrowseItemNextF)val;
      break;
    }
  }

 done:
  if(status!=LPGDV_OK) {
    if(*params) free(*params);
    if(*menu_item_params) free(*menu_item_params);
    if(*column_params) free(*column_params);
  }
  return status;

# undef BEFORE_VA
}

/*****************************************************************************/
/* Example Main function */

/* this contains a lot of Micah's file dialog code, as example. It will
   eventually disappear and serve as inspiration in the displets code.
*/

const char *_dirview(pgfilter filefilter, const char *pattern,
		    const char *deffile, int flags, const char *title) {

  pghandle wTB, wOk, wCancel, wUp;
  struct pgEvent evt;
  struct filepickdata dat;
  int w,h;
  char *p;

  /* If this is the first invocation, use the current directory */
  if (!dirview_dir[0])
    getcwd(dirview_dir,FILEMAX);

  /* Store picker data */
  memset(&dat,0,sizeof(dat));
  dat.flags = flags;
  dat.filefilter = filefilter;
  dat.pattern = pattern;

  /********* Set up dialog box and top-level widgets */

  pgEnterContext();

  /* Size the dialog box ourselves. On a handheld this should
   * be basically as large as possible. Just forcing it to take
   * the whole screen at 1280x1024 would be a little awkward.
   * Since using absolute coordinates would be a Bad Thing, size
   * it relative to the default font.
   *
   * Usually the letter 'a' is 6x10 pixels, so multiplying by 66x40 would
   * make the dialog box about 400x400. That will suck up the whole
   * screen on 320x240 or 240x320 or smaller handhelds, but on a
   * desktop-sized screen it will look very reasonable.
   * Measuring relative to a font is useful, but make sure to use the
   * PG_FSTYLE_FLUSH flag!
   */
  pgEnterContext();
  pgSizeText(&w,&h,pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_FLUSH),
			     pgNewString("a"));  /* A good metric */
  pgLeaveContext();
#if 0
  pgNewPopup(w*66,h*40);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString(title),
	      PG_WP_TRANSPARENT,0,
	      /* The PG_WP_STATE property is fun :) */
	      PG_WP_STATE,PGTH_O_LABEL_DLGTITLE,
	      0);
#endif

  /* Special fonts for directories and links */
  dat.fDirectory = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_BOLD);
  dat.fLink      = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_ITALIC);
  dat.fHeading   = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_BOLD |
			     PG_FSTYLE_UNDERLINE);
  
  /* Make containers for the directory and file. They are ok without
   * containers, but it looks better putting them in toolbars.
   */
  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);
  dat.wDirectory = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_TOP,
	      0);

  dat.wScroll = pgNewWidget(PG_WIDGET_SCROLL,0,0);
  dat.wFileList = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  pgSetWidget(dat.wScroll,
	      PG_WP_BIND,dat.wFileList,
	      0);

  /* Put the file and directory in their toolbars */
  if (flags & PG_FILE_FIELD) {
    dat.wFile = pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wTB);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE,PG_S_ALL,
		0);
    if (deffile)
      pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString(deffile),0);
  }

  wUp = pgNewWidget(PG_WIDGET_BUTTON,
		    PG_DERIVE_INSIDE,dat.wDirectory);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString(".."),
	      0);
  dat.wDirectory = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);

  /********** Widgets */

  wCancel = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString("Cancel"),
	      PG_WP_HOTKEY,PGKEY_ESCAPE,
	      PG_WP_BITMAP,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_CANCEL),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_CANCEL_MASK),
	      0);

  wOk = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString((flags&PG_FILE_SAVEBTN) ? 
				     "Save" : "Open"),
	      PG_WP_HOTKEY,PGKEY_RETURN,
	      PG_WP_BITMAP,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);

  /********** Run the dialog */

  /* Set up the default directory in it's own directory context */
  pgEnterContext();
  dirview_setdir(&dat);

  /* If we have a field, focus it first */
  if (dat.wFile)
    pgFocus(dat.wFile);

  for (;;) {
    evt = *pgGetEvent();
    
    /* Buttons... */

    if (evt.from==wCancel)
      break;

    else if (evt.from==wOk) {
      struct stat st;

      /* Put together a full path for the final file name */
      if (dat.wFile)
	dat.sFileName = pgGetWidget(dat.wFile,PG_WP_TEXT);
      dirview_fullpath(pgGetString(dat.sFileName));
    
      /* Validate it */
      
      if (stat(dirview_buf,&st) && (flags & (PG_FILE_MUSTEXIST | 
						PG_FILE_MUSTREAD  |
						PG_FILE_MUSTWRITE))) {
	pgMessageDialog("Error","The selected file does not exist",0);
	continue;
      }

      if (flags & PG_FILE_MUSTREAD) {
	int file = open(dirview_buf,O_RDONLY);
	if (file<0) {
	  pgMessageDialogFmt("Error",0,"Error reading file:\n%s",
			     strerror (errno));
	  continue;
	}
	close(file);
      }

      if (flags & PG_FILE_MUSTWRITE) {
	int file = open(dirview_buf,O_RDWR);
	if (file<0) {
	  pgMessageDialogFmt("Error",0,"Error writing file:\n%s",
			     strerror (errno));
	  continue;
	}
	close(file);
      }

      /* Good! We're all done */
      break;
    }

    else if (evt.from==wUp) {
      /* Go up one level: chop off the slash until we have only one */
      p = strrchr(dirview_dir,'/');
      if (p && p[1]) {
	if (p==dirview_dir)
	  p[1] = 0;
	else
	  p[0] = 0;
	dirview_setdir(&dat);
      }
    }

    else if (evt.from==dat.wDirectory && evt.type==PG_WE_PNTR_DOWN)
#if 0
      dirview_pathmenu(&dat);
#else
      ;
#endif

    /* 'Tis a file we hope? */
    else if (pgGetPayload(evt.from)==FILETAG) {

      /* File or directory? */
      if (pgGetWidget(evt.from,PG_WP_FONT)==dat.fDirectory) {
	/* Directory => Follow the directory */
	dirview_fullpath(pgGetString(pgGetWidget(evt.from,PG_WP_TEXT)));
	strcpy(dirview_dir,dirview_buf);
	dirview_setdir(&dat);

	/* No valid file */
	dat.sFileName = 0;
      }
      else {
	/* File => Select the file */
	
	dat.sFileName = pgGetWidget(evt.from, PG_WP_TEXT);
	if (dat.wFile)
	  pgSetWidget(dat.wFile,PG_WP_TEXT,dat.sFileName,0);
      }
    }
  }

  /* Destroy the directory context and dialog context */
  pgLeaveContext();
  pgLeaveContext();

  /* Don't have to return much, eh? */
  if (evt.from==wCancel)
    return NULL;
  
  /* Return the assembled full path */
  return dirview_buf;
}


/*****************************************************************************/

typedef enum {
  NONE, QUIT, RELOAD,
} NextAction;

/*****************************************************************************/

static NextAction next_action = NONE;

static int
location_activate(struct pgEvent *evt)
{
  ENTER("location_activate");
  const char* str = pgGetString(pgGetWidget(evt->from,PG_WP_TEXT));

  DPRINTF(">>> location is [%s]\n", str);

  next_action = RELOAD;

  LEAVE;
  return 0;
}

/*****************************************************************************/

#define FREE(var)          { if(var) free((void*)(var)); var=0; }
#define REASSIGN(var, val) { FREE(var); var=val; }

// TODO: pack these in instance variables
static const char*          this_url = 0;
static const LpgdvProtocol* this_protocol = 0;
static const char*          this_username = 0;
static const char*          this_password = 0;
static const char*          this_host = 0;
static int                  this_port = 0;
static const char*          this_dir = 0;
static void *               this_pdata = 0;

/*****************************************************************************/

static void
dir_leave_old(void)
{
  ENTER("dir_leave_old()");
  if(this_protocol && this_dir) {
    DPRINTF("leaving [%s]\n", this_dir);
    this_protocol->dir_leave_f(this_pdata, this_dir);
  }
  FREE(this_dir);
  LEAVE;
}

static void
site_leave_old(void)
{
  ENTER("site_leave_old()");
  dir_leave_old();

  if(this_protocol && this_host) {
    DPRINTF("leaving [%s]:%d\n", this_host, this_port);
    this_protocol->site_leave_f(this_pdata, this_host, this_port);
  }
  FREE(this_host);

  this_port = 0;

  FREE(this_username);
  FREE(this_password);
  LEAVE;
}

static void
protocol_leave_old(void)
{
  ENTER("protocol_leave_old()");
  site_leave_old();
  if(this_protocol) {
    DPRINTF("leaving [%s]\n", this_protocol->name);
    this_protocol->protocol_uninit_f(this_pdata);
  }
  this_protocol = 0;
  LEAVE;
}

static int
protocol_enter_new(const LpgdvProtocol* protocol)
{
  ENTER("protocol_enter_new()");
  int res = 0;
  if(protocol) {
    DPRINTF("entering [%s]\n", protocol->name);
    this_pdata = protocol->protocol_init_f();
    this_protocol = 0;
    FREE(this_username);
    FREE(this_password);
    FREE(this_host);
    FREE(this_dir);
    this_port = 0;
    if(this_pdata) {
      this_protocol = protocol;
      res = 1;
    }
  }
  LEAVE;
  return res;
}

static int
site_enter_new(const char* site_name, int port,
	       const char* username, const char* password)
{
  ENTER("site_enter_new()");
  int res = 0;
  if(site_name && this_protocol) {
    DPRINTF("entering [%s];%d\n", site_name, port);
    DPRINTF("login: usr=[%s] pwd=[%s]\n", username?username:"<>",
	    password?password:"<>");
    res = this_protocol->site_enter_f(this_pdata, site_name, port,
				      username, password);
    FREE(this_username);
    FREE(this_password);
    FREE(this_host);
    FREE(this_dir);
    this_port = 0;
    if(res) {
      this_host = strdup(site_name);
      this_port = port;
      this_username = strdup(username);
      this_password = strdup(password);
    }
  }
  LEAVE;
  return res;
}

static int
dir_enter_new(const char* dir_name)
{
  ENTER("dir_enter_new()");
  int res = 0;
  if(dir_name && this_protocol) {
    DPRINTF("entering [%s]\n", dir_name);
    res = this_protocol->dir_enter_f(this_pdata, dir_name);
    this_dir = res ? strdup(dir_name) : 0;
  }
  LEAVE;
  return res;
}

/*****************************************************************************/

static LpgdvStatus
reload(const char* url, char** corrected_url, Parameters* params)
{
  ENTER("reload()");


  LpgdvStatus status = LPGDV_OK;
  char* url_copy = 0;
  const char* protocol_name;
  const char* address;
  char* p;
  const LpgdvProtocol *protocol;
  LpgdvSplittedUrl su;
  int ok;
  const char* dir_name = 0;
  const char* item_name = 0;
  LpgdvMimeTypeId mt_id;

  if(url==0 || strlen(url)==0) {
    DPRINTF("empty url\n");
    status = LPGDV_ERROR_INVALID_URL;
    goto done;
  }

  /* check against previous url */
  if(this_url && !strcmp(url, this_url)) {
    DPRINTF("same url\n");
    goto done;
  }

  /* remember this url */
  REASSIGN(this_url, strdup(url));
  DPRINTF("url = [%s]\n", url);

  /* split url into protocol an address */
  url_copy = strdup(url);
  p = strchr(url_copy, ':');
  if(p) {
    *p = 0;
    protocol_name = url_copy;
    address = p+1;
  }
  else if(params->protocol_default){
    protocol_name = params->protocol_default;
    address = url_copy;
  }
  else {
    protocol_name = 0;
  }

  protocol = lpgdv_protocol_find(protocol_name);
  if(protocol==0) {
    WARNF("libpgdirview: "__FILE__": warning: unknown protocol: %s\n",
	  protocol_name);
    status = LPGDV_ERROR_UNKNOWN_PROTOCOL;
    goto done;
  }

  DPRINTF("protocol = [%s]\n", protocol->name);
  DPRINTF("address = [%s]\n", address);

  ok = lpgdv_protocol_split_url(address, protocol, &su);
  if(!ok) {
    WARNF("warning: lpgdv_protocol_split_url() failed\n");
    status = LPGDV_ERROR_INVALID_URL;
    goto done;
  }

  DPRINTF("URL[%s] splitted:\n", url);
  DPRINTF("  username  = [%s]\n", su.username ? su.username : "<>");
  DPRINTF("  password  = [%s]\n", su.password ? su.password : "<>");
  DPRINTF("  host      = [%s]\n", su.host ? su.host : "<>");
  DPRINTF("  port      = %d\n",   su.port);
  DPRINTF("  path      = [%s]\n", su.path ? su.path : "<>");
  DPRINTF("  arguments = [%s]\n", su.arguments ? su.arguments : "<>");

  REASSIGN(dir_name, protocol->getdirname_f(su.path));
  REASSIGN(item_name, protocol->getitemname_f(su.path));

  DPRINTF("  dir       = [%s]\n", dir_name ? dir_name : "<>");
  DPRINTF("  item      = [%s]\n", item_name ? item_name : "<>");

  mt_id = lpgdv_mime_type_id_of(item_name, -1);

  DPRINTF("object=[%s] id=%d mime-type=[%s]\n",
	  item_name ? item_name : "<>",
	  mt_id, lpgdv_mime_type_name_of(mt_id));

  /* another protocol ? */
  if(this_protocol==0 || this_protocol!=protocol) {
    protocol_leave_old();
    protocol_enter_new(protocol);
  }

  /* another host ? */
  if(protocol->has_host) {
    if(this_host==0 ||
       !protocol->is_same_site_f(this_host, su.host) ||
       this_port != su.port) {
      site_leave_old();
      site_enter_new(su.host, su.port, su.username, su.password);
    }
  }

  /* another dir ? */
  DPRINTF(":::[%s][%s]\n", this_dir, dir_name);
  if(this_dir==0 || !protocol->is_same_dir_f(this_dir, dir_name)) {
    dir_leave_old();
    dir_enter_new(dir_name);
  }
  
  /* TODO:
   - init protocol
   - enter site
   - enter dir
   - call function that browse dir for each item
   - leave dir
   - leave site
   - uninit protocol
  */
  //#warning todo ...



 done:
  FREE(url_copy);
  FREE(dir_name);
  FREE(item_name);
  LEAVE;
  return status;
}

/*****************************************************************************/
/* Main function */

const char *dirview(const char *view_name,
		    const char *url,
		    ...)
{
  pghandle wTB, wOk, wCancel, wUp;
  struct pgEvent evt;
  struct filepickdata dat;
  int w,h;
  char *p;
  Parameters* params = 0;
  MenuItemParams* menu_item_params = 0;
  ColumnParams* column_params = 0;
  LpgdvStatus status;

  {
    va_list va;
    va_start(va, url);
    status = va_parse_params(&params,
			     &menu_item_params,
			     &column_params,
			     va);
    va_end(va);
  }

  dump_params(params);
  dump_menuitemparams(params->menu_items_nb, menu_item_params);
  dump_columnparams(params->columns_nb, column_params);

  /********* Set up dialog box and top-level widgets */
  pgEnterContext(); {

    /* Size the dialog box ourselves */
    pgEnterContext(); {
      pgSizeText(&w,&h,pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_FLUSH),
		 pgNewString("a"));  /* A good metric */
    } pgLeaveContext();

    /* Special fonts for directories and links */
    dat.fDirectory = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_BOLD);
    dat.fLink      = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_ITALIC);
    dat.fHeading   = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_BOLD |
			       PG_FSTYLE_UNDERLINE);

    /* Make containers for the directory and file. They are ok without
     * containers, but it looks better putting them in toolbars.
     */
    if(params->location_visible) {
      wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
      pgSetWidget(PGDEFAULT,
		  PG_WP_SIDE,PG_S_BOTTOM,
		  0);
    }

    dat.wDirectory = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE,PG_S_TOP,
		0);

    dat.wScroll = pgNewWidget(PG_WIDGET_SCROLL,0,0);
    dat.wFileList = pgNewWidget(PG_WIDGET_BOX,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE,PG_S_ALL,
		0);
    pgSetWidget(dat.wScroll,
		PG_WP_BIND,dat.wFileList,
		0);

    /* Put the file and directory in their toolbars */
    /*
    if (flags & PG_FILE_FIELD) {
      dat.wFile = pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wTB);
      pgSetWidget(PGDEFAULT,
		  PG_WP_SIDE,PG_S_ALL,
		  0);
      if (deffile)
	pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString(deffile),0);
    }
    */
    if(!params->list_do_include_parent) {
      /* [..] button */
      wUp = pgNewWidget(PG_WIDGET_BUTTON,
			PG_DERIVE_INSIDE,dat.wDirectory);
      pgSetWidget(PGDEFAULT,
		  PG_WP_TEXT,pgNewString(".."),
		0);
    }

    /* Title bar */
    if(params->title_is_visible) {
      dat.wDirectory = params->list_do_include_parent
	? pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE,dat.wDirectory)
	: pgNewWidget(PG_WIDGET_BUTTON, 0,0);
      pgSetWidget(PGDEFAULT,
		  PG_WP_SIDE,PG_S_ALL,
		  PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
		  0);
      if(params->title) {
	pgSetWidget(PGDEFAULT,
		    PG_WP_TEXT,pgNewString(params->title),
		    0);
      }
    }

    /* url */
    if(params->location_visible) {
      dat.wFile = pgNewWidget(params->location_visible==EDITABLE
			      ? PG_WIDGET_FIELD : PG_WIDGET_LABEL,
			      PG_DERIVE_INSIDE,wTB);
      pgSetWidget(PGDEFAULT, PG_WP_SIDE, PG_S_ALL, 0);

      if(params->location_visible==EDITABLE) {
	pgBind(PGDEFAULT,PG_WE_ACTIVATE,&location_activate,NULL);
      }

      if(url) {
	pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString(url),0);
      }

    }

    //pgEventLoop();
    next_action = RELOAD; /* initial load */
    do {
      switch(next_action) {
      case NONE:
	break;
      case RELOAD:
	{
	  const char* loc = url;
	  if(dat.wFile) loc = pgGetString(pgGetWidget(dat.wFile,PG_WP_TEXT));
	  reload(loc, 0, params);
	}
	break;
      }

      pgDispatchEvent(pgGetEvent());
    } while (next_action != QUIT);


  } pgLeaveContext();


  /* Store picker data */
  return dirview_buf;
}

/* The End */

// main lib function call args:
// - instance_name
// - user_data
// - fd_handler(fd, user_data, lib_data)
// - taglist
//
// additional function calls:
// - int handle_fd(fd)
//
// functions handlers can call:
// - reparse_taglist(this, taglist)
//
// app messages:
// - set_location(str)
// - select_item(num)
// - unselect_item(num)
// - select_all
// - unselect_all
// - click(num)
// - quit

