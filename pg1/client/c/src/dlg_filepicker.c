/* $Id$
 *
 * dlg_filepicker.c - Display a dialog box the user can use to select
 *                    a file to open or save. It is customizable with flags
 *                    or a user-defined filter function
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
 * 
 * 
 * 
 * 
 */

#include "clientlib.h"

#include <string.h>     /* strcpy(), strcat() */
#include <stdlib.h>     /* For getenv() */
#include <sys/types.h>  /* opendir(), readdir(), etc */
#include <dirent.h>
#include <sys/stat.h>   /* stat() */
#include <unistd.h>
#include <errno.h>      /* Check errors when validating a filename */
#include <ctype.h>

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
char filepicker_dir[FILEMAX];

/* Static buffer for returning the file name */
char filepicker_buf[FILEMAX];

/* A structure for passing data to filepicker_setdir */
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

int filepicker_filter(struct filepickdata *dat, const char *name,
		      struct stat *st);
void filepicker_fullpath(const char *file);
void filepicker_setdir(struct filepickdata *dat);
int  filepicker_compare(const void *a, const void *b);
void filepicker_pathmenu(struct filepickdata *dat);

/******************************** Utility functions */

/* This is just like pgMenuFromString, except that we use
 * '/' as the separator and the items are listed backwards,
 * ending with a bare '/'. When the user chooses a path, 
 * set the dialog's directory
 */
void filepicker_pathmenu(struct filepickdata *dat) {
  char *p;
  char *items = filepicker_dir;
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
      
      /* Payload is the index in filepicker_dir to set to zero
       * if it is chosen */
      i = items-filepicker_dir;
      if (i>1)
	i--;
      pgSetPayload(PGDEFAULT,i);
    }
    
  } while (*p);

  /* Run the menu */
  ret = pgGetPayload(pgGetEvent()->from);
  pgLeaveContext();
  if (ret>0) {
    filepicker_dir[ret] = 0;
    filepicker_setdir(dat);
  }
  else if (ret<0) {
    /* Update selected file string */
    if (dat->wFile)
      dat->sFileName = pgGetWidget(dat->wFile,PG_WP_TEXT);
    
    switch (-ret) {

    case 1 :    /* Home */
      filepicker_dir[FILEMAX-1] = 0;
      strncpy(filepicker_dir,getenv("HOME"),FILEMAX-1);
      filepicker_setdir(dat);
      break;
      
    case 2:     /* New Directory */
      str = pgInputDialog("New Directory", "Name:",0);
      if (str) {
	filepicker_fullpath(pgGetString(str));
	pgDelete(str);
	
	if (mkdir(filepicker_buf,0777)) 
	  pgMessageDialogFmt("Error",0,"Unable to create directory:\n%s",
			     filepicker_buf);
	else
	  filepicker_setdir(dat);
    }
      break;
      
    case 3:     /* Delete */
      if (dat->sFileName) {
	filepicker_fullpath(pgGetString(dat->sFileName));
	if (pgMessageDialog("Delete?",filepicker_buf,
			    PG_MSGBTN_YES | PG_MSGBTN_NO) == PG_MSGBTN_YES) {
	  
	  if (unlink(filepicker_buf))
	    pgMessageDialogFmt("Error",0,"Unable to delete file:\n%s",
			       filepicker_buf);
	  else
	    filepicker_setdir(dat);
	}
      }
      break;
      
    case 4:     /* Rename */
      if (dat->sFileName) {
	filepicker_fullpath(pgGetString(dat->sFileName));
	str = pgInputDialog("Rename File",filepicker_buf,0);
	if (str) {
	  char *oldname, *newname;
	  oldname = strdup(filepicker_buf);
	  newname = pgGetString(str);
	  if (newname[0] != '/') {
	    filepicker_fullpath(newname);
	    newname = filepicker_buf;
	  }
	  
	  if (rename(oldname,newname))
	    pgMessageDialogFmt("Error",0,"Unable to rename file:\n%s\nto\n%s",
			       oldname,newname);
	  else
	    filepicker_setdir(dat);
	  
	  free(oldname);
	}
      }
      break;
            
    }
  }
}

/* Sort the files, in a way that should make sense to users.
 * Directories always come before files. Case is ignored, punctuation
 * is ignored. If the file has a trailing number, the numbers are sorted
 * numerically.
 */
int  filepicker_compare(const void *a, const void *b) {
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

/* Utility that, given the current filters and flags, determines if a file
 * should be visible. Returns nonzero if the file should be visible. */
int filepicker_filter(struct filepickdata *dat, const char *name,
		      struct stat *st) {
 
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

/* Given a filename, constructs it's full path in filepicker_buf
 * careful to avoid overflowing any buffers. 
 */
void filepicker_fullpath(const char *file) {
  int len = strlen(filepicker_dir);
  strcpy(filepicker_buf,filepicker_dir);
  if (len<(FILEMAX-1) && filepicker_buf[len-1]!='/') {
    strcat(filepicker_buf,"/");
    len--;
  }
  strncat(filepicker_buf,file,FILEMAX-1-len);
}

/* Utility to populate the dialog box with the current directory's files */
void filepicker_setdir(struct filepickdata *dat) {
  DIR *d;
  struct dirent *arthur;     /* Dirent, Arthur Dirent... */
  struct stat st;
  struct filenode *names, *p;
  int total = 0, count, i;
  int itemheight;
  char *s;
  pghandle font;
  pghandle wNameBoxP, wSizeBoxP;
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
  s = strrchr(filepicker_dir,'/');
  if (s[1]) s++;
  pgSetWidget(dat->wDirectory,PG_WP_TEXT,pgNewString(s),0);

  /* Select the relevant entries, store them, and sort them. This is just
   * like what scandir() does, but scandir doesn't allow passing extra
   * data to the selection function. I suppose another reason to avoid
   * scandir() is that uClibc doesn't yet implement it correctly...
   */

  /* first just count the files... */
  d = opendir(filepicker_dir);
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
    filepicker_fullpath(arthur->d_name);
    lstat(filepicker_buf,&st);
    
    if (filepicker_filter(dat,arthur->d_name,&st)) {
    
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
    qsort(names,count,sizeof(struct filenode),&filepicker_compare);
    
    /* Make Columns */
    wNameBoxP = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,dat->wFileList);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,1,
		PG_WP_SIDE,PG_S_LEFT,
		0);
    wSizeBoxP = pgNewWidget(PG_WIDGET_BOX,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,1,
		PG_WP_SIDE,PG_S_ALL,
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
      wSizeBoxP = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_AFTER,wSizeBoxP);
      pgSetWidget(PGDEFAULT,
		  PG_WP_TEXT,pgNewString(buf),
		  PG_WP_ALIGN,PG_A_LEFT,
		  PG_WP_SIZE,itemheight,
		  PG_WP_TRANSPARENT,0,
		  PG_WP_STATE,PGTH_O_LISTITEM,
		  0);
    }
  }

  /* Free the memory! */
  free(names);
}

/******************************** Date Picker */

const char *pgFilePicker(pgfilter filefilter, const char *pattern,
			 const char *deffile, int flags, const char *title) {

  pghandle wTB, wOk, wCancel, wUp, wPopup;
  struct pgEvent evt;
  struct filepickdata dat;
  int w,h;
  char *p;

  /* If this is the first invocation, use the current directory */
  if (!filepicker_dir[0])
    getcwd(filepicker_dir,FILEMAX);

  /* Store picker data */
  memset(&dat,0,sizeof(dat));
  dat.flags = flags;
  dat.filefilter = filefilter;
  dat.pattern = pattern;

  /********* Set up dialog box and top-level widgets */

  pgEnterContext();
  wPopup = pgDialogBox(title);

  /* Size the dialog box ourselves. FIXME: This sizing is crufty :P
   */
  pgEnterContext();
  pgSizeText(&w,&h,pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_FLUSH),pgNewString("a"));
  pgLeaveContext();
  pgSetWidget(wPopup,
	      PG_WP_WIDTH, w*40,
	      PG_WP_HEIGHT, h*30,
	      0);

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
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
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
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);

  /********** Run the dialog */

  /* Set up the default directory in it's own directory context */
  pgEnterContext();
  filepicker_setdir(&dat);

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
      filepicker_fullpath(pgGetString(dat.sFileName));
    
      /* Validate it */
      
      if (stat(filepicker_buf,&st) && (flags & (PG_FILE_MUSTEXIST | 
						PG_FILE_MUSTREAD  |
						PG_FILE_MUSTWRITE))) {
	pgMessageDialog("Error","The selected file does not exist",0);
	continue;
      }

      if (flags & PG_FILE_MUSTREAD) {
	int file = open(filepicker_buf,O_RDONLY);
	if (file<0) {
	  pgMessageDialogFmt("Error",0,"Error reading file:\n%s",
			     strerror (errno));
	  continue;
	}
	close(file);
      }

      if (flags & PG_FILE_MUSTWRITE) {
	int file = open(filepicker_buf,O_RDWR);
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
      p = strrchr(filepicker_dir,'/');
      if (p && p[1]) {
	if (p==filepicker_dir)
	  p[1] = 0;
	else
	  p[0] = 0;
	filepicker_setdir(&dat);
      }
    }

    else if (evt.from==dat.wDirectory && evt.type==PG_WE_PNTR_DOWN)
      filepicker_pathmenu(&dat);

    /* 'Tis a file we hope? */
    else if (pgGetPayload(evt.from)==FILETAG) {

      /* File or directory? */
      if (pgGetWidget(evt.from,PG_WP_FONT)==dat.fDirectory) {
	/* Follow the directory */
	
	filepicker_fullpath(pgGetString(pgGetWidget(evt.from,PG_WP_TEXT)));
	strcpy(filepicker_dir,filepicker_buf);
	filepicker_setdir(&dat);

	/* No valid file */
	dat.sFileName = 0;
      }
      else {
	/* Select the file */
	
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
  return filepicker_buf;
}

/* The End */
