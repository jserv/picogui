/* $Id$
 * 
 * thobjtab.c - Searches themes already in memory,
 *              and loads themes in memory
 *
 * MAGIC runtime
 * Magic Algorithm for General Interface Configurability
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
 */

#include <pgserver/common.h>

#include <pgserver/timer.h>
#include <pgserver/svrtheme.h>
#include <pgserver/divtree.h>
#include <pgserver/requests.h>
#include <pgserver/pgstring.h>
#include <pgserver/configfile.h>
#include <pgserver/os.h>
#include <picogui/theme.h>

#include <stdio.h>   /* for NULL */
#include <stdlib.h>  /* for rand() */
#include <string.h>

/***************** Data */

/* Indexed by a PGTH_O_* constant, the value is the theme object's
 * parent (also a PGTH_O_* constant).  PGTH_O_DEFAULT is magical,
 * and is always inherited from the next theme in the list. 
 *
 * I should probably make some kind of nifty inheritance chart for
 * this :)
 *
 * This used to be in a seperate file, but a possible bug in egcs
 * (compiler or linker) forced me to move it in here.
 */
u16 thobj_ancestry[PGTH_ONUM] = {

  /* #  Theme object                 Parent              */

  /* 0  PGTH_O_DEFAULT               */ 0,                       /* (magical, this is ignored) */
  /* 1  PGTH_O_BASE_INTERACTIVE      */ PGTH_O_DEFAULT,
  /* 2  PGTH_O_BASE_CONTAINER        */ PGTH_O_DEFAULT,
  /* 3  PGTH_O_BUTTON                */ PGTH_O_BASE_INTERACTIVE,
  /* 4  PGTH_O_BUTTON_HILIGHT        */ PGTH_O_BUTTON,
  /* 5  PGTH_O_BUTTON_ON             */ PGTH_O_BUTTON_HILIGHT,
  /* 6  PGTH_O_TOOLBAR               */ PGTH_O_BASE_CONTAINER,
  /* 7  PGTH_O_SCROLL                */ PGTH_O_BASE_INTERACTIVE,
  /* 8  PGTH_O_SCROLL_HILIGHT        */ PGTH_O_SCROLL,
  /* 9  PGTH_O_INDICATOR             */ PGTH_O_BASE_DISPLAY,
  /* 10 PGTH_O_PANEL                 */ PGTH_O_BASE_TLCONTAINER,
  /* 11 PGTH_O_PANELBAR              */ PGTH_O_BASE_INTERACTIVE,
  /* 12 PGTH_O_POPUP                 */ PGTH_O_BASE_TLCONTAINER,
  /* 13 PGTH_O_BACKGROUND            */ PGTH_O_DEFAULT,
  /* 14 PGTH_O_BASE_DISPLAY          */ PGTH_O_DEFAULT,
  /* 15 PGTH_O_BASE_TLCONTAINER      */ PGTH_O_DEFAULT,
  /* 16 PGTH_O_THEMEINFO             */ PGTH_O_DEFAULT,
  /* 17 PGTH_O_LABEL                 */ PGTH_O_BASE_DISPLAY,
  /* 18 PGTH_O_FIELD                 */ PGTH_O_BASE_DISPLAY,
  /* 19 PGTH_O_BITMAP                */ PGTH_O_BASE_DISPLAY,
  /* 20 PGTH_O_SCROLL_ON             */ PGTH_O_SCROLL_HILIGHT,
  /* 21 PGTH_O_LABEL_SCROLL          */ PGTH_O_LABEL,
  /* 22 PGTH_O_PANELBAR_HILIGHT      */ PGTH_O_PANELBAR,
  /* 23 PGTH_O_PANELBAR_ON           */ PGTH_O_PANELBAR_HILIGHT,
  /* 24 PGTH_O_BOX                   */ PGTH_O_BASE_CONTAINER,
  /* 25 PGTH_O_LABEL_DLGTITLE        */ PGTH_O_LABEL,
  /* 26 PGTH_O_LABEL_DLGTEXT         */ PGTH_O_LABEL,
  /* 27 PGTH_O_CLOSEBTN              */ PGTH_O_BASE_PANELBTN,
  /* 28 PGTH_O_CLOSEBTN_ON           */ PGTH_O_CLOSEBTN_HILIGHT,
  /* 29 PGTH_O_CLOSEBTN_HILIGHT      */ PGTH_O_CLOSEBTN,
  /* 30 PGTH_O_BASE_PANELBTN         */ PGTH_O_BUTTON,
  /* 31 PGTH_O_ROTATEBTN             */ PGTH_O_BASE_PANELBTN,
  /* 32 PGTH_O_ROTATEBTN_ON          */ PGTH_O_ROTATEBTN_HILIGHT,
  /* 33 PGTH_O_ROTATEBTN_HILIGHT     */ PGTH_O_ROTATEBTN,
  /* 34 PGTH_O_ZOOMBTN               */ PGTH_O_BASE_PANELBTN,
  /* 35 PGTH_O_ZOOMBTN_ON            */ PGTH_O_ZOOMBTN_HILIGHT,
  /* 36 PGTH_O_ZOOMBTN_HILIGHT       */ PGTH_O_ZOOMBTN,
  /* 37 PGTH_O_POPUP_MENU            */ PGTH_O_POPUP,
  /* 38 PGTH_O_POPUP_MESSAGEDLG      */ PGTH_O_POPUP,
  /* 39 PGTH_O_MENUITEM              */ PGTH_O_BASE_INTERACTIVE,
  /* 40 PGTH_O_MENUITEM_HILIGHT      */ PGTH_O_MENUITEM,
  /* 41 PGTH_O_CHECKBOX              */ PGTH_O_BASE_INTERACTIVE,
  /* 42 PGTH_O_CHECKBOX_HILIGHT      */ PGTH_O_CHECKBOX,     
  /* 43 PGTH_O_CHECKBOX_ON           */ PGTH_O_CHECKBOX_HILIGHT,
  /* 44 PGTH_O_FLATBUTTON            */ PGTH_O_DEFAULT,
  /* 45 PGTH_O_FLATBUTTON_HILIGHT    */ PGTH_O_BUTTON_HILIGHT,     
  /* 46 PGTH_O_FLATBUTTON_ON         */ PGTH_O_BUTTON_ON,
  /* 47 PGTH_O_LISTITEM              */ PGTH_O_MENUITEM,
  /* 48 PGTH_O_LISTITEM_HILIGHT      */ PGTH_O_LISTITEM,
  /* 49 PGTH_O_LISTITEM_ON           */ PGTH_O_MENUITEM_HILIGHT,
  /* 50 PGTH_O_CHECKBOX_ON_NOHILIGHT */ PGTH_O_CHECKBOX_ON,
  /* 51 PGTH_O_SUBMENUITEM           */ PGTH_O_MENUITEM,
  /* 52 PGTH_O_SUBMENUITEM_HILIGHT   */ PGTH_O_MENUITEM_HILIGHT,
  /* 53 PGTH_O_RADIOBUTTON           */ PGTH_O_CHECKBOX,
  /* 54 PGTH_O_RADIOBUTTON_HILIGHT   */ PGTH_O_CHECKBOX_HILIGHT,
  /* 55 PGTH_O_RADIOBUTTON_ON        */ PGTH_O_CHECKBOX_ON,
  /* 56 PGTH_O_RADIOBUTTON_ON_NOHILIGHT */ PGTH_O_CHECKBOX_ON_NOHILIGHT,
  /* 57 PGTH_O_TEXTBOX               */ PGTH_O_BASE_DISPLAY,
  /* 58 PGTH_O_TERMINAL              */ PGTH_O_BASE_DISPLAY,
  /* 59 PGTH_O_LIST                  */ PGTH_O_BASE_INTERACTIVE,
  /* 60 PGTH_O_MENUBUTTON            */ PGTH_O_BUTTON,
  /* 61 PGTH_O_MENUBUTTON_ON         */ PGTH_O_MENUBUTTON_HILIGHT,
  /* 62 PGTH_O_MENUBUTTON_HILIGHT    */ PGTH_O_MENUBUTTON,
  /* 63 PGTH_O_LABEL_HILIGHT         */ PGTH_O_LABEL,
  /* 64 PGTH_O_BOX_HILIGHT           */ PGTH_O_BOX,
  /* 65 PGTH_O_INDICATOR_H           */ PGTH_O_INDICATOR,
  /* 66 PGTH_O_INDICATOR_V           */ PGTH_O_INDICATOR,
  /* 67 PGTH_O_SCROLL_H              */ PGTH_O_SCROLL,
  /* 68 PGTH_O_SCROLL_V              */ PGTH_O_SCROLL,
  /* 69 PGTH_O_SCROLL_H_ON           */ PGTH_O_SCROLL_H_HILIGHT,
  /* 70 PGTH_O_SCROLL_H_HILIGHT      */ PGTH_O_SCROLL_H,
  /* 71 PGTH_O_SCROLL_V_ON           */ PGTH_O_SCROLL_V_HILIGHT,
  /* 72 PGTH_O_SCROLL_V_HILIGHT      */ PGTH_O_SCROLL_V,
  /* 73 PGTH_O_PANELBAR_H            */ PGTH_O_PANELBAR,
  /* 74 PGTH_O_PANELBAR_V            */ PGTH_O_PANELBAR,
  /* 75 PGTH_O_PANELBAR_H_ON         */ PGTH_O_PANELBAR_H_HILIGHT,
  /* 76 PGTH_O_PANELBAR_H_HILIGHT    */ PGTH_O_PANELBAR_H,
  /* 77 PGTH_O_PANELBAR_V_ON         */ PGTH_O_PANELBAR_V_HILIGHT,
  /* 78 PGTH_O_PANELBAR_V_HILIGHT    */ PGTH_O_PANELBAR_V,
  /* 79 PGTH_O_TEXTEDIT              */ PGTH_O_TEXTBOX,
  /* 80 PGTH_O_MANAGEDWINDOW         */ PGTH_O_PANEL,
  /* 81 PGTH_O_TAB_PAGE              */ PGTH_O_BASE_CONTAINER,
  /* 82 PGTH_O_TAB_BAR               */ PGTH_O_TOOLBAR,
  /* 83 PGTH_O_TAB                   */ PGTH_O_BUTTON,
  /* 84 PGTH_O_TAB_HILIGHT           */ PGTH_O_BUTTON_HILIGHT,
  /* 85 PGTH_O_TAB_ON                */ PGTH_O_BUTTON_ON,
  /* 86 PGTH_O_TAB_ON_NOHILIGHT      */ PGTH_O_TAB_ON,
};

struct pgmemtheme *memtheme;

/* Store handles of all server-loaded themes so they can be freed */
static struct themenode {
  handle h;
  struct themenode *next;
} *themelist = NULL;

/***************** Theme searching */

/* Given a theme object, returns the theme object parent's ID 
 */
s16 thobj_parent(s16 id) {
  s16 parent = PGTH_O_DEFAULT;
  int prop;
  static int lock = 0;

  if (lock) {
    /* This is being called from within the theme_lookup() below.
     * We don't want to have inheritance for the PGTH_P_PARENT
     * property anyway, so return zero.
     */
    return 0;
  }
  lock++;
  
  /* For pre-defined theme objects, use our default inheritance */
  if (id >= 0 && id < PGTH_ONUM)
    parent = thobj_ancestry[id];

  /* Object can override its inheritance with PGTH_P_PARENT */
  prop = theme_lookup(id, PGTH_P_PARENT);
  if (prop)
      parent = prop;

  lock--;
  return parent;
}

struct pgmemtheme_thobj *find_thobj(struct pgmemtheme *th, u16 id) {
  u16 i;
  struct pgmemtheme_thobj *tlist = theme_thobjlist(th);
  if (!th) return NULL;
  
  /* FIXME: use binary search here for better speed */
  for (i=0;i<th->num_thobj;i++,tlist++)
    if (tlist->id == id) return tlist;
  return NULL;
}

struct pgmemtheme_prop *find_prop(struct pgmemtheme_thobj *tho, u16 id) {
  u16 i;
  struct pgmemtheme_prop *plist;

  if (!tho) return NULL;
  plist = tho->proplist.ptr;

  /* FIXME: use binary search here for better speed */
  for (i=0;i<tho->num_prop;i++,plist++)
    if (plist->id == id) return plist;
  return NULL;
}

#ifdef CONFIG_ANIMATION
/* Use the specified value as the 'ticks' theme property instead of
 * retrieving it with os_getticks(). This lets pgserver be used to generate
 * non-realtime animation.
 */
u32 ticks_override;
void pg_ticks_override(u32 ticks) {
  ticks_override = ticks;
}
#endif

/* Look for the given property, starting at 'object'
 * returns the property's 'data' member */
u32 theme_lookup(u16 object, u16 property) {
  struct pgmemtheme *ptheme = memtheme;
  struct pgmemtheme_thobj *pobj;
  struct pgmemtheme_prop *pprop;
  u16 obj;

  while (ptheme) {
    obj = object;

    /* Search this theme, using the theme object hierarchy */
    pobj = find_thobj(ptheme,obj);
    pprop = NULL;
    while ((!pobj) || (!(pprop = find_prop(pobj,property)))) {
      if (!obj) break;
      obj = thobj_parent(obj);
      pobj = find_thobj(ptheme,obj);
    }
    
    /* Got it? */
    if (pprop) {
#ifdef DEBUG_THEME
       printf("theme_lookup(0x%04X,0x%04X) = 0x%08X\n",
	      object,property,pprop->data);
#endif
       return pprop->data;
    }
       
    /* Try another theme */
    ptheme = ptheme->next;
  }

  /************** Theme defaults */

#ifdef DEBUG_THEME
       printf("theme_lookup(0x%04X,0x%04X) = (not found)\n",
	      object,property);
#endif
   
  /* This defines defaults for values the pgserver itself needs, so we can
     function even without a theme loaded */

  switch (property) {
    
  case PGTH_P_BGCOLOR:          return 0xFFFFFF;
  case PGTH_P_FGCOLOR:          return 0x000000;
  case PGTH_P_FONT:             return res[PGRES_DEFAULT_FONT];
  case PGTH_P_ALIGN:            return PG_A_CENTER;
  case PGTH_P_BITMAPSIDE:       return PG_S_LEFT;
  case PGTH_P_BITMAPMARGIN:     return 2;
  case PGTH_P_MARGIN:           return 2;
  case PGTH_P_WIDTH:            return 12;
  case PGTH_P_HEIGHT:           return 15;
  case PGTH_P_SIDE:
    switch (object) {
    case PGTH_O_PANEL:    return PG_S_BOTTOM;
    case PGTH_O_TOOLBAR:  return PG_S_TOP;
    default:              return PG_S_LEFT;
    }
  case PGTH_P_HOTKEY_OK:        return PGKEY_RETURN;
  case PGTH_P_HOTKEY_CANCEL:    return PGKEY_ESCAPE;
  case PGTH_P_HOTKEY_YES:       return PGKEY_y;
  case PGTH_P_HOTKEY_NO:        return PGKEY_n;
  case PGTH_P_HOTKEY_UP:        return PGKEY_UP;
  case PGTH_P_HOTKEY_DOWN:      return PGKEY_DOWN;
  case PGTH_P_HOTKEY_LEFT:      return PGKEY_LEFT;
  case PGTH_P_HOTKEY_RIGHT:     return PGKEY_RIGHT;
  case PGTH_P_HOTKEY_ACTIVATE:  return PGKEY_SPACE;
  case PGTH_P_HOTKEY_NEXT:      return PGKEY_TAB;
  case PGTH_P_ATTR_DEFAULT:     return 0x07;
  case PGTH_P_ATTR_CURSOR:      return 0xF0;
  case PGTH_P_TEXTCOLORS:       return res[PGRES_DEFAULT_TEXTCOLORS];
  case PGTH_P_TIME_ON:          return 250;
  case PGTH_P_TIME_OFF:         return 125;
  case PGTH_P_TIME_DELAY:       return 500;
#ifdef CONFIG_ANIMATION
  case PGTH_P_TICKS:            return ticks_override ? ticks_override : os_getticks();
  case PGTH_P_RANDOM:           return rand();
#endif
  case PGTH_P_CURSORBITMAP:     return res[PGRES_DEFAULT_CURSORBITMAP];
  case PGTH_P_CURSORBITMASK:    return res[PGRES_DEFAULT_CURSORBITMASK];
  case PGTH_P_CURSOR_WIDTH:     return 2;

  default:
    return 0;       /* Couldn't hurt? */
  }
}

/* Given a divnode, it uses the 'build' member function to rebuild
   the node's groplist */
void div_rebuild(struct divnode *d) {
   struct gropctxt c;

#ifdef DEBUG_VIDEO
   printf("div_rebuild: div %p at %d,%d,%d,%d\n",
	  d, d->r.x, d->r.y, d->r.w, d->r.h);
#endif

   if (d->build) {

     /* Unless it's a raw build, clear the groplist. */
     if (!(d->flags & DIVNODE_RAW_BUILD))
       grop_free(&d->grop);

     gropctxt_init(&c,d);     
     (*d->build)(&c,d->state,d->owner);
     
     /* Unless this is a raw build, set redraw flags */
     if (!(d->flags & DIVNODE_RAW_BUILD)) {
       d->flags |= DIVNODE_NEED_REDRAW;
       if (d->owner)
	 d->owner->dt->flags |= DIVTREE_NEED_REDRAW;
     }
   }

   /* If this node has children, rebuild them, etc.. */
   if (d->div)
     d->div->flags |= DIVNODE_NEED_REBUILD;
   if (d->next)
     d->next->flags |= DIVNODE_NEED_REBUILD;

   d->flags &= ~DIVNODE_NEED_REBUILD;
}

/* This is used in div_setstate() to compare theme objects.
 * It finds the theme object as high as possible in the theme
 * hierarchy that will act equivalently to the specified theme object.
 */
u16 trace_thobj(u16 obj) {
   struct pgmemtheme *ptheme;
   
   while (obj) {
   
      /* If this object is defined in any loaded themes, return it */
      for (ptheme=memtheme;ptheme;ptheme=ptheme->next)
	if (find_thobj(ptheme,obj))
	  return obj;
      
      /* Next object in the hierarchy */
      obj = thobj_parent(obj);
   }
   
   return obj;
}

/* Change a divnode's state, and update the necessary things. */
void div_setstate(struct divnode *d,u16 state,bool force) {
   u16 prevstate = d->state;
   d->state = state;
   
   if (!force) {          /* Try to optimize it */
      
      if (state==prevstate)
	return;             /* exact same state */
      
      /* Try to determine if the new state and old state look exactly the same,
       * and avoid redrawing if possible. This isn't as easy as I thought it would
       * be. Simply comparing the fillstyle handle for both state's bgfill
       * properties didn't work, as most themes use changes in color, bitmap,
       * or other parameters to indicate state changes. The next thing that comes
       * to mind is to check whether the theme object is exactly the same. This
       * doesn't work either, because due to the hierarchial nature of the theme,
       * different theme objects may be used for different properties.
       * 
       * Though I don't think it's optimal, this is the best method I could come
       * up with. (and it seems to work pretty well) It uses the trace_thobj()
       * function (defined above) to check whether the new theme object is any
       * different than the currently selected theme object.
       * trace_thobj() finds the theme object as high as possible in the theme
       * hierarchy that will act equivalently to the specified theme object.
       */
      if (trace_thobj(state) == trace_thobj(prevstate))
	return;
   }
   
   div_rebuild(d);
   
   /* state changes are caused by interaction with the user, and should
    be reported ASAP back to the user */
   update(force ? NULL : d,1);
}

/* Small build function for widgets that only need a background */
void build_bgfill_only(struct gropctxt *c, u16 state, struct widget *self) {
  exec_fillstyle(c,state,PGTH_P_BGFILL);
}

/***************** Custos theme objects */

u16 custom_thobj_id(struct pgstring *name) {
  static u16 next_id = 0;
  u16 x;

  /* If we've already got this object, use the same ID */
  if (find_named_thobj(name,&x))
    return x;

  /* Search for an available ID between PGTH_ONUM and PGTH_O_CUSTOM-1 */
  do {
    next_id++;

    if (next_id < PGTH_ONUM)
      next_id = PGTH_ONUM;
    
    if (next_id >= PGTH_O_CUSTOM)
      next_id = PGTH_ONUM;
  } while (!thobj_id_available(next_id));
    
  return next_id;
}

int thobj_id_available(s16 id) {
  struct pgmemtheme *ptheme;
  
  for (ptheme=memtheme;ptheme;ptheme=ptheme->next)
    if (find_thobj(ptheme,id))
      return 0;

  return 1;
}

/* Search all theme objects in all themes for a PGTH_P_NAME property
 * matching the given string. If it's found, this puts its id in "*id"
 * and returns nonzero.
 */
int find_named_thobj(const struct pgstring *name, s16 *id) {
  struct pgmemtheme *ptheme;
  struct pgmemtheme_thobj *pobj;
  struct pgmemtheme_prop *pprop;
  struct pgstring *thisname;
  int i;

  for (ptheme=memtheme;ptheme;ptheme=ptheme->next)
    for (pobj=theme_thobjlist(ptheme),i=0;i<ptheme->num_thobj;pobj++,i++)
      if ((pprop = find_prop(pobj,PGTH_P_NAME)))
	if (!iserror(rdhandle((void**)&thisname,PG_TYPE_PGSTRING,-1,pprop->data)))
	  if (thisname && !pgstring_cmp(thisname,name)) {
	    *id = pobj->id;
	    return 1;
	  }
     
  return 0;
}

/***************** Theme loading */

void theme_divtree_update(struct pgmemtheme *th) {
  if (th->requires_full_update) {
    struct divtree *tree;
    
    /* To make the them take effect:
     *  - call widget_resize() on all widgets
     *  - recalc all divnodes
     *  - rebuild all applicable divnodes
     *  - redraw all divtree layers
     *  - reclip all popups
     */
    resizeall();
    for (tree=dts->top;tree;tree=tree->next) {
      tree->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC | DIVNODE_NEED_REBUILD;
      tree->flags |= DIVTREE_NEED_RECALC | DIVTREE_ALL_REDRAW | DIVTREE_CLIP_POPUP;
    }
  }
}

g_error theme_load(handle *h,int owner,const u8 *themefile,
		   u32 themefile_len) {
  g_error e;
  struct pgtheme_header *hdr;
  u32 sum32;
  u32 mysum32 = 0;
  unsigned char *cp;
  u32 c;
  u16 i,j;
  unsigned long heaplen;
  unsigned char *heap;
  struct pgtheme_thobj *thop;
  struct pgmemtheme_thobj *mthop,*thobjarray;
  u32 themefile_remaining = themefile_len;
  const char *themefile_start = themefile;
  struct pgtheme_prop *propp;
  struct pgmemtheme_prop *mpropp;
  struct pgmemtheme *th;
  struct pgstring *objname;

  /* Get the header */
  if (themefile_remaining < sizeof(struct pgtheme_header))
    return mkerror(PG_ERRT_FILEFMT,83);    /* No header! */
  hdr = (struct pgtheme_header *) themefile;

  /* Validate magic number */
  if (hdr->magic[0] != 'P' ||
      hdr->magic[1] != 'G' ||
      hdr->magic[2] != 't' ||
      hdr->magic[3] != 'h')
    return mkerror(PG_ERRT_FILEFMT,80);    /* Bad magic number */

  /* Store and zero the checksum to exclude it from the
     checksum calculations */
  sum32  = ntohl(hdr->file_sum32);
  hdr->file_sum32  = 0;

  /* Verify the checksum */
  cp = (unsigned char *) themefile;
  for (c=0;c<themefile_remaining;c++)
    mysum32 += *(cp++);
  if (mysum32 != sum32)
    return mkerror(PG_ERRT_FILEFMT,82);

  /* Convert the byte orders in the header 
     (make sure to do this after the checksum!) */
  hdr->file_len    = ntohl(hdr->file_len); 
  hdr->file_ver    = ntohs(hdr->file_ver);
  hdr->num_tags    = ntohs(hdr->num_tags); 
  hdr->num_thobj   = ntohs(hdr->num_thobj);
  hdr->num_totprop = ntohs(hdr->num_totprop);

  /* Validate length */
  if (hdr->file_len != themefile_len)
    return mkerror(PG_ERRT_FILEFMT,81); /* Length mismatch */

  /* Increment pointers from the header
     (have to do this here to not interfere 
     with header verification) */
  themefile += sizeof(struct pgtheme_header);
  themefile_remaining -= sizeof(struct pgtheme_header);

  /* Calculate the length of our internal theme heap */
  heaplen = 
    sizeof(struct pgmemtheme) +
    sizeof(struct pgmemtheme_thobj) * hdr->num_thobj +
    sizeof(struct pgmemtheme_prop)  * hdr->num_totprop;

  /* Allocate it */
  e = g_malloc((void**) &heap,heaplen);
  errorcheck;
  memset(heap,0,heaplen);

  /* Make a handle to it */
  if (iserror(e = mkhandle(h,PG_TYPE_THEME,owner,heap))) {
    g_free(heap);
    return e;
  }

#ifdef DEBUG_THEME
  printf("Allocated an internal theme heap: %lu bytes\n",heaplen);
#endif

  /* Stick a memtheme header on the new heap */
  th = (struct pgmemtheme *) heap;
  heap += sizeof(struct pgmemtheme);
  heaplen -= sizeof(struct pgmemtheme);  

  /* Fill in the pgmemetheme header */
  th->num_thobj = hdr->num_thobj;
  th->requires_full_update = 0;

  /* Allocate thobj array on the heap */
  mthop = thobjarray = (struct pgmemtheme_thobj *) heap;
  heap += sizeof(struct pgmemtheme_thobj) * hdr->num_thobj;
  heaplen -= sizeof(struct pgmemtheme_thobj) * hdr->num_thobj;

  /* Transcribe the thobj array */
  for (i=0;i<hdr->num_thobj;i++,mthop++) {
    
    /* Grab a new thobj from the file */
    thop = (struct pgtheme_thobj *) themefile;
    themefile += sizeof(struct pgtheme_thobj);
    if (themefile_remaining < sizeof(struct pgtheme_thobj)) {
      handle_free(owner,*h);
      return mkerror(PG_ERRT_FILEFMT,84);   /* Unexpected EOF */
    }
    themefile_remaining -= sizeof(struct pgtheme_thobj);

    /* Transcribe, changing byte order */
    mthop->id = ntohs(thop->id);
    mthop->num_prop = ntohs(thop->num_prop);
    mthop->proplist.offset = ntohl(thop->proplist);
  }

  /* Transcribe and link the properties */
  mthop = thobjarray;   /* Set the memory theme object pointer */
  /* For each theme object... */
  for (i=0;i<hdr->num_thobj;i++,mthop++) {
    objname = NULL;

    /* Validate the offset, and make a pointer */
    if (mthop->proplist.offset > (themefile_len - 
				  sizeof(struct pgtheme_prop) * 
				  mthop->num_prop)) {
      handle_free(owner,*h);
      return mkerror(PG_ERRT_FILEFMT,86);  /* Out-of-range offset */
    }
    propp = (struct pgtheme_prop *) (themefile_start + mthop->proplist.offset);

    /* Allocate a property list on the heap */
    mpropp = (struct pgmemtheme_prop *) heap;
    heap += sizeof(struct pgmemtheme_prop) * mthop->num_prop;
    if (heaplen < (sizeof(struct pgmemtheme_prop) * mthop->num_prop)) {
      handle_free(owner,*h);
      return mkerror(PG_ERRT_FILEFMT,85); /* Heap overflow */
    }
    heaplen -= sizeof(struct pgmemtheme_prop) * mthop->num_prop;

    /* Link the memory theme object to our current location
       in the heap, so we can find its property list later */
    mthop->proplist.ptr = mpropp;

    /* Each property... */
    for (j=0;j<mthop->num_prop;j++,mpropp++,propp++) {
      
      /* Transcribe it */
      mpropp->id = ntohs(propp->id);
      mpropp->data = ntohl(propp->data);

      /* check if setting this property would cause a full refresh of
	 the display */
      switch (mpropp->id) {
      case PGTH_P_NAME:
      case PGTH_P_CURSORBITMAP:
      case PGTH_P_CURSORBITMASK:
	/* these properties don't force a refresh. */
	break;

      default:
	th->requires_full_update = 1;
	break;
      }

      /* Loaders */
      switch (mpropp->loader = ntohs(propp->loader)) {

      case PGTH_LOAD_REQUEST: {
	struct request_data r;
	memset(&r,0,sizeof(r));
	
	/* Validate offset, and load pointers */
	if (mpropp->data >= (themefile_len-sizeof(struct pgrequest))) {
	  handle_free(owner,*h);
	  return mkerror(PG_ERRT_FILEFMT,86); /* Out-of-range  */
	}
	if (mpropp->data & 3) {
	   handle_free(owner,*h);
	   return mkerror(PG_ERRT_FILEFMT,100); /* not aligned */
	}

	r.in.req  = (struct pgrequest *) (themefile_start + mpropp->data);
	r.in.data = (void *) (r.in.req+1);
	r.in.owner = owner;

	/* Dispatch the request packet */
	if (iserror(e = request_exec(&r))) {
	  handle_free(owner,*h);
	  return e;
	}
	if (r.out.free_response_data)
	  g_free(r.out.response_data);
	mpropp->data = r.out.ret;

	/* Group the handle so it is cleaned up at the same time */
	handle_group(owner,*h,mpropp->data,-1);

      } break;

      /* Copy and Findthobj loaders handled later to support forward references */
      case PGTH_LOAD_COPY:   
      case PGTH_LOAD_FINDTHOBJ:
      case PGTH_LOAD_NONE:
	break;
	
      default:
	handle_free(owner,*h);
	return mkerror(PG_ERRT_FILEFMT,87); /* Unknown loader */

      }

      /* If that was the 'name' property, save it for below... */
      if (mpropp->id == PGTH_P_NAME)
	rdhandle((void**)&objname,PG_TYPE_PGSTRING,owner,mpropp->data);
    }
    
    /* If this was a custom theme object, give it an ID now */
    if (mthop->id == PGTH_O_CUSTOM)
      mthop->id = custom_thobj_id(objname);

  } /* Next theme object */

  /* Add to the linked list */
  th->next = memtheme;
  memtheme = th;

  /* Process copy loaders and findthobj loaders. This can't be done at the
   * same time as the other loaders because it needs to reference
   * information that may not exist yet */

  for (i=0,mthop = thobjarray;i<hdr->num_thobj;i++,mthop++)
    for (j=0,mpropp = mthop->proplist.ptr;j<mthop->num_prop;j++,mpropp++)
      switch (mpropp->loader) {

      case PGTH_LOAD_COPY:
	mpropp->data = theme_lookup(mpropp->data >> 16,mpropp->data & 0xFFFF);
	break;

      case PGTH_LOAD_FINDTHOBJ:
	/* Validate offset */
	if (mpropp->data >= (themefile_len-1)) {
	  handle_free(owner,*h);
	  return mkerror(PG_ERRT_FILEFMT,86); /* Out-of-range  */
	}
	if (mpropp->data & 3) {
	   handle_free(owner,*h);
	   return mkerror(PG_ERRT_FILEFMT,100); /* not aligned */
	}

	{
	  s16 id;
	  if (find_named_thobj(pgstring_tmpwrap(themefile_start + mpropp->data), &id))
	    mpropp->data = id;
	  else
	    mpropp->data = 0;
	}
	break;

      }

  /* Reload the mouse cursors */
  cursor_retheme();

  /* Reload global hotkeys from the theme */
  reload_hotkeys();
  
  /* Set flags for an update due to loading/unloading the theme */
  theme_divtree_update(th);

  /* Notify apps of the theme insertion */
  th->h = *h;
  post_event_global(PG_NWE_THEME_INSERTED,NULL,th->h,NULL);

  return success;
}

void theme_remove(struct pgmemtheme *th) {
  struct pgmemtheme *p;

  /* Unlink from the linked list */
  if (memtheme == th)
    memtheme = memtheme->next;
  else if (memtheme) {
    p = memtheme;
    while (p->next) {
      if (p->next == th) {
	p->next = p->next->next;
	break;
      }
      p = p->next;
    }
  }

  /* Reload the mouse cursor */
  cursor_retheme();
  
  /* Set flags for an update due to loading/unloading the theme */
  theme_divtree_update(th);

  /* Notify apps of the theme removal */
  post_event_global(PG_NWE_THEME_REMOVED,NULL,th->h,NULL);

  g_free(th);
}

/* Reload themes according to the ones set in the config database 
 * FIXME: This should be replaced with something more flexible that can be
 *        modified at runtime by apps.
 */
g_error reload_initial_themes(void) {
  const char *constthemes, *themedir;
  char *themes,*str,*fullpath;
  char *tok;
  g_error e;
  FILE *f;
  u32 themesize;
  u8 *themebuf;
  struct themenode *tn;

  /* If we don't have a divtree yet, skip this reload */
  if (!dts)
    return success;

  theme_shutdown();
  themedir = get_param_str("pgserver","themedir","");

  if ((constthemes = get_param_str("pgserver","themes",NULL))) {
    str = themes = strdup(constthemes);
    while ((tok = strtok(str," \t"))) {

      /* Make a full path for this theme */
      e = g_malloc((void**)&fullpath, strlen(themedir)+strlen(tok)+2);
      errorcheck;
      strcpy(fullpath,themedir);
      if (*fullpath)
	strcat(fullpath,"/");
      strcat(fullpath,tok);

      /* Make a node in our theme list so we can delete the handle later */
      e = g_malloc((void**)&tn, sizeof(struct themenode));
      errorcheck;
      tn->next = themelist;
      themelist = tn;

      /* Load the theme */
      if (!(f = fopen(fullpath,"rb")))
	return mkerror(PG_ERRT_IO,109);       /* Can't find a theme file */
      fseek(f,0,SEEK_END);
      themesize = ftell(f);
      fseek(f,0,SEEK_SET);
      e = g_malloc((void**)&themebuf,themesize);
      errorcheck;
      fread(themebuf,1,themesize,f);
      fclose(f);
      e = theme_load(&tn->h,-1,themebuf,themesize);
      errorcheck;

      g_free(themebuf);
      g_free(fullpath);
      str = NULL;
    }
    free(themes);
  }
  
  /* Default hotkeys, from the theme */
  reload_hotkeys();

  return success; 
}

/* Free all memory used by server-loaded themes */
void theme_shutdown(void) {
  struct themenode *tn, *dead_tn;

  for (tn=themelist;tn;) {
    dead_tn = tn;
    tn = tn->next;
    handle_free(-1,dead_tn->h);
    g_free(dead_tn);
  }
}

/* The End */
