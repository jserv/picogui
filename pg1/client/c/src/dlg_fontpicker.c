/* $Id$
 *
 * dlg_datepicker.c - Implementation of the pgFontPicker function, allowing
 *                    the user to choose any installed font
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

/* Because there are many very similar widgets created here, put flags
 * in the payload to indicate the widget's type
 */

#define F_STYLEBOX      0x80000000
#define F_NAME          0x40000000
#define F_SIZE          0x20000000
#define F_MASK          0x00FFFFFF

/* The sample text string */
#define SAMPLETEXT      "ABC abc"

/* Structure for saving font style info */
struct fontpicker_node {
  char name[40];
  u16 size;
  u32 flags;
  struct fontpicker_node *next;
};

/* Font comparison for the insertion sort. Sort first by font name,
 * then by size. 
 *
 * Returns nonzero if a > b
 */
int fontpicker_compare(struct fontpicker_node *a, struct fontpicker_node *b) {
  int i = strcmp(a->name,b->name);
  if (i)
    return i>0;
  return a->size>b->size;
}

/* Little helper function to create a style button */
void fontpicker_style(pghandle str, u32 style, s16 rship,
		      pghandle parent) {
  pgNewWidget(PG_WIDGET_BUTTON,rship,parent);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,str,
	      PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE,
	      PG_WP_FONT,pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | style),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgSetPayload(PGDEFAULT,F_STYLEBOX | style);
}

pghandle pgFontPicker(const char *title) {
  pghandle wTB,wNames,wSizes,wStyles,wSample,wOk,wCancel,wSampleBox;
  pghandle wNameHeading,wSizeHeading;
  pghandle wLeftBox,wRightBox,wScroll;
  pghandle fSelected = 0, sA;
  int maxSize = 0;
  char name[40];
  char buf[10];         /* For making the size into a string */
  char *s;
  u32 flags, fontflags;
  u16 size;
  struct pgEvent evt;
  u32 id;
  struct fontpicker_node *fontlist = NULL;
  struct fontpicker_node *n, **where;
  int i;

  pgEnterContext();
  pgDialogBox(title);

  /********* Read all the fonts from the server and sort them */

  for (i=0;(n=alloca(sizeof(struct fontpicker_node))) &&
	 pgGetFontStyle(i,n->name,&n->size,NULL,&n->flags);i++) {

    /* Record the maximum size */
    if (n->size > maxSize)
      maxSize = n->size;
    
    /* Insertion sort */
    where = &fontlist;
    while ((*where) && fontpicker_compare(n,*where))
      where = &((*where)->next);
    n->next = *where;
    *where = n;
  }

  /********* Create top-level widgets */

  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);

  wSampleBox = pgNewWidget(PG_WIDGET_BOX,0,0);

  wStyles = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_RIGHT,
	      0);

  wRightBox = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TRANSPARENT,1,
	      0);
  wLeftBox = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TRANSPARENT,1,
	      0);

  wScroll = pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_INSIDE,wLeftBox);
  wNames = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  pgSetWidget(wScroll,PG_WP_BIND,wNames,0);
  wNameHeading = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wNames);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Name:"),
	      PG_WP_SIDE,PG_S_TOP,
	      PG_WP_TRANSPARENT,0,
	      0);

  wScroll = pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_INSIDE,wRightBox);
  wSizes = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  pgSetWidget(wScroll,PG_WP_BIND,wSizes,0);
  wSizeHeading = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wSizes);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Size:"),
	      PG_WP_SIDE,PG_S_TOP,
	      PG_WP_TRANSPARENT,0,
	      0);

  /********* Create widgets */

  /* Standard toolbar buttons */
  wOk = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Ok"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,PGKEY_RETURN,
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);
  wCancel = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Cancel"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,PGKEY_ESCAPE,
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_CANCEL),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_CANCEL_MASK),
	      0);

  /* Set up the sample text area */
  pgSetWidget(wSampleBox,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_SIZE,maxSize + (maxSize>>1),
	      0);
  wSample = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wSampleBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TEXT,pgNewString(SAMPLETEXT),
	      0);

  /* Style buttons */
  sA = pgNewString("Abc");
  fontpicker_style(sA,PG_FSTYLE_BOLD,PG_DERIVE_INSIDE,wStyles);
  fontpicker_style(sA,PG_FSTYLE_ITALIC,0,0);
  fontpicker_style(sA,PG_FSTYLE_UNDERLINE,0,0);
  fontpicker_style(sA,PG_FSTYLE_STRIKEOUT,0,0);
  fontpicker_style(sA,PG_FSTYLE_DOUBLEWIDTH,0,0);
  
  /********* Add font styles */

  for (s=NULL,i=0,n=fontlist;n;i++,s=n->name,n=n->next) {
    /* Skip non-unique style names */
    if (s && !strcmp(s,n->name))
      continue;

    pgNewWidget(PG_WIDGET_LISTITEM,
		i ? 0 : PG_DERIVE_AFTER, i ? 0 : wNameHeading);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,pgNewString(n->name),
		0);
    pgSetPayload(PGDEFAULT,F_NAME | i);
  }

  /********* Run it */

  flags = fontflags = 0;
  size = 0;
  name[0] = 0;

  /* Another context for the size lists */
  pgEnterContext();

  for (;;) {
    evt = *pgGetEvent();
    id = pgGetPayload(evt.from);

    /* Ok or cancel button? */

    if (evt.from == wOk)
      break;

    if (evt.from == wCancel) {
      fSelected = 0;
      break;
    }

    /* Other widgets determined by payload flags */

    if (id & F_STYLEBOX)
      flags ^= id & F_MASK;

    else if (id & F_NAME) {
      strcpy(name,pgGetString(pgGetWidget(evt.from,PG_WP_TEXT)));

      /* Clear the size list context */
      pgLeaveContext();
      pgEnterContext();

      for (s=NULL,i=0,n=fontlist;n;i++,n=n->next) {
	if (i < (id & F_MASK))
	  continue;
	
	/* Exit when the font name changes */
	if (s && strcmp(s,n->name))
	  break;
	
	sprintf(buf,"%d",n->size);
	pgNewWidget(PG_WIDGET_LISTITEM,
		    s ? 0 : PG_DERIVE_AFTER, s ? 0 : wSizeHeading);
	pgSetWidget(PGDEFAULT,
		    PG_WP_TEXT,pgNewString(buf),
		    0);
	pgSetPayload(PGDEFAULT,F_SIZE | n->size);

	fontflags = n->flags;
	size = n->size;
	s = n->name;
      }
    }

    else if (id & F_SIZE)
      size = id & F_MASK;

    /* Update sample */
    if (fSelected)
      pgDelete(fSelected);
    fSelected = pgNewFont(name,size,flags | fontflags);
    pgSetWidget(wSample,PG_WP_FONT,fSelected,0);
  }

  /* Send the selected font up 2 contexts and return it
   *
   * (one for sizes, one for everything else)
   */
  if (fSelected)
    pgChangeContext(fSelected,-2);
  pgLeaveContext();
  pgLeaveContext();
  return fSelected;
}

/* The End */


























