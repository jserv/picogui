/* $Id: memtheme.c,v 1.29 2001/05/10 20:57:30 micahjd Exp $
 * 
 * thobjtab.c - Searches themes already in memory,
 *              and loads themes in memory
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/svrtheme.h>
#include <pgserver/divtree.h>
#include <pgserver/pgnet.h>
#include <picogui/theme.h>

#include <stdio.h>  /* for NULL */

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
unsigned short thobj_ancestry[PGTH_ONUM] = {

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
};

struct pgmemtheme *memtheme;

/***************** Theme searching */

struct pgmemtheme_thobj *find_thobj(struct pgmemtheme *th,unsigned short id) {
  unsigned short i;
  struct pgmemtheme_thobj *tlist = theme_thobjlist(th);
  if (!th) return NULL;
  
  /* FIXME: use binary search here for better speed */
  for (i=0;i<th->num_thobj;i++,tlist++)
    if (tlist->id == id) return tlist;
  return NULL;
}

struct pgmemtheme_prop *find_prop(struct pgmemtheme_thobj *tho,unsigned short id) {
  unsigned short i;
  struct pgmemtheme_prop *plist;

  if (!tho) return NULL;
  plist = tho->proplist.ptr;

  /* FIXME: use binary search here for better speed */
  for (i=0;i<tho->num_prop;i++,plist++)
    if (plist->id == id) return plist;
  return NULL;
}

/* Look for the given property, starting at 'object'
 * returns the property's 'data' member */
unsigned long theme_lookup(unsigned short object,
			   unsigned short property) {
  struct pgmemtheme *ptheme = memtheme;
  struct pgmemtheme_thobj *pobj;
  struct pgmemtheme_prop *pprop;
  unsigned short obj;

  while (ptheme) {
    obj = object;

    /* Search this theme, using the theme object hierarchy */
    pobj = find_thobj(ptheme,obj);
    pprop = NULL;
    while ((!pobj) || (!(pprop = find_prop(pobj,property)))) {
      if ((!obj) || obj>=PGTH_ONUM) break;
      obj = thobj_ancestry[obj];
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
    
  case PGTH_P_BGCOLOR:        return 0xFFFFFF;
  case PGTH_P_FGCOLOR:        return 0x000000;
  case PGTH_P_FONT:           return defaultfont;
  case PGTH_P_ALIGN:          return PG_A_CENTER;
  case PGTH_P_BITMAPSIDE:     return PG_S_LEFT;
  case PGTH_P_BITMAPMARGIN:   return 2;
  case PGTH_P_MARGIN:         return 2;
  case PGTH_P_WIDTH:          return 12;
  case PGTH_P_HEIGHT:         return 15;
  case PGTH_P_STRING_OK:      return string_ok;
  case PGTH_P_STRING_CANCEL:  return string_cancel;
  case PGTH_P_STRING_YES:     return string_yes;
  case PGTH_P_STRING_NO:      return string_no;
  case PGTH_P_SIDE:           return PG_S_LEFT;
  case PGTH_P_HOTKEY_OK:      return PGKEY_RETURN;
  case PGTH_P_HOTKEY_CANCEL:  return PGKEY_ESCAPE;
  case PGTH_P_HOTKEY_YES:     return PGKEY_y;
  case PGTH_P_HOTKEY_NO:      return PGKEY_n;

  default:
    return 0;       /* Couldn't hurt? */
  }
}

/* Given a divnode, it uses the 'build' member function to rebuild
   the node's groplist */
void div_rebuild(struct divnode *d) {
   struct gropctxt c;
   
   if (!d->build) return;
   if (!(d->owner && d->owner->rawbuild)) {
      grop_free(&d->grop);
      gropctxt_init(&c,d);
   }
   (*d->build)(&c,d->state,d->owner);
   if (!(d->owner && d->owner->rawbuild)) {
      d->flags |= DIVNODE_NEED_REDRAW;
      if (d->owner)
	d->owner->dt->flags |= DIVTREE_NEED_REDRAW;
   }
}

/* This is used in div_setstate() to compare theme objects.
 * It finds the theme object as high as possible in the theme
 * hierarchy that will act equivalently to the specified theme object.
 */
u16 trace_thobj(u16 obj) {
   struct pgmemtheme *ptheme;
   struct pgmemtheme_thobj *pobj;
   
   while (obj && (obj<PGTH_ONUM)) {
   
      /* If this object is defined in any loaded themes, return it */
      for (ptheme=memtheme;ptheme;ptheme=ptheme->next)
	if (find_thobj(ptheme,obj))
	  return obj;
      
      /* Next object in the hierarchy */
      obj = thobj_ancestry[obj];
   }
   
   return obj;
}

/* Change a divnode's state, and update the necessary things. */
void div_setstate(struct divnode *d,u16 state) {
  u16 prevstate = d->state;
  if (state==prevstate)
     return;             /* exact same state */
  d->state = state;

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

  div_rebuild(d);

  /* state changes are caused by interaction with the user, and should
     be reported ASAP back to the user */
  if (d->owner && d->owner->dt == dts->top)
    update(NULL,1);
}

/* Small build function for widgets that only need a background */
void build_bgfill_only(struct gropctxt *c,unsigned short state,struct widget *self) {
  exec_fillstyle(c,state,PGTH_P_BGFILL);
}

/***************** Theme loading */

g_error theme_load(handle *h,int owner,char *themefile,
		   unsigned long themefile_len) {
  g_error e;
  struct pgtheme_header *hdr;
  unsigned long sum32;
  unsigned long mysum32 = 0;
  unsigned char *cp;
  unsigned long c;
  unsigned short i,j;
  unsigned long heaplen;
  unsigned char *heap;
  struct pgtheme_thobj *thop;
  struct pgmemtheme_thobj *mthop,*thobjarray;
  unsigned long themefile_remaining = themefile_len;
  char *themefile_start = themefile;
  struct pgtheme_prop *propp;
  struct pgmemtheme_prop *mpropp;
  struct pgmemtheme *th;

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
  printf("Allocated an internal theme heap: %d bytes\n",heaplen);
#endif

  /* Stick a memtheme header on the new heap */
  th = (struct pgmemtheme *) heap;
  heap += sizeof(struct pgmemtheme);
  heaplen -= sizeof(struct pgmemtheme);  

  /* Fill in the pgmemetheme header */
  th->num_thobj = hdr->num_thobj;

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

      /* Loaders */
      switch (mpropp->loader = ntohs(propp->loader)) {

      case PGTH_LOAD_REQUEST: {
	struct pgrequest *req;
	void *data;
	int fatal;
	
	/* Validate offset, and load pointers */
	if (mpropp->data >= (themefile_len-sizeof(struct pgrequest))) {
	  handle_free(owner,*h);
	  return mkerror(PG_ERRT_FILEFMT,86); /* Out-of-range  */
	}
#ifdef UCLINUX
	if (mpropp->data & 3) {
	   handle_free(owner,*h);
	   return mkerror(PG_ERRT_FILEFMT,100); /* not aligned */
	}
#endif  
	req = (struct pgrequest *) (themefile_start + mpropp->data);
	req->type = ntohs(req->type);
	req->size = ntohl(req->size);
	data = (void *) (req+1);
	if (req->type>PGREQ_UNDEF) req->type = PGREQ_UNDEF;

#ifdef DEBUG_THEME
	printf("Theme request loader executing: type = %d, size = %d\n",req->type,req->size);
#endif
	
	/* Dispatch the request packet */
	if (iserror(e = (*rqhtab[req->type])(owner,req,data,&mpropp->data,&fatal))) {
	  handle_free(owner,*h);
	  return e;
	}

	/* Group the handle so it is cleaned up at the same time */
	handle_group(owner,*h,mpropp->data);

      } break;

      case PGTH_LOAD_COPY:   /* Copy loaders are handled later */
      case PGTH_LOAD_NONE:
	break;
	
      default:
	handle_free(owner,*h);
	return mkerror(PG_ERRT_FILEFMT,87); /* Unknown loader */

      }

    }
    
  } /* Next theme object */

  /* Add to the linked list */
  th->next = memtheme;
  memtheme = th;

  /* Process copy loaders. This can't be done at the same time as the other
     loaders because it needs to reference information that may not exist yet */

  for (i=0,mthop = thobjarray;i<hdr->num_thobj;i++,mthop++)
    for (j=0,mpropp = mthop->proplist.ptr;j<mthop->num_prop;j++,mpropp++)
      if (mpropp->loader == PGTH_LOAD_COPY) {
	unsigned long d = mpropp->data;
	/* Sometimes this confuses the compiler :( */
	mpropp->data = theme_lookup(d >> 16,d & 0xFFFF);
      }

  /* Reload the mouse cursor */
  appmgr_loadcursor(PGTH_O_DEFAULT);
   
  /* Schedule a global recalc (Yikes!) so it takes effect */
  resizeall();
  dts->top->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
  dts->top->flags |= DIVTREE_NEED_RECALC;

  return sucess;
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

  /* Schedule a global recalc (Yikes!) so it takes effect */
  if (!in_shutdown) {
     /* Reload the mouse cursor */
     appmgr_loadcursor(PGTH_O_DEFAULT);

     resizeall();
     dts->top->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
     dts->top->flags |= DIVTREE_NEED_RECALC;
  }

  g_free(th);
}

/* The End */
