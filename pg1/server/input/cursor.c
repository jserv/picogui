/* $Id$
 *
 * cursor.c - Cursor abstraction and multiplexing layer 
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
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>

/* List of all cursors */
struct cursor *cursor_list;

void cursor_widgetunder(struct cursor *crsr);
void r_cursor_widgetunder(struct cursor *crsr, struct divnode *div, int x, int y);
void cursor_change_under(struct cursor *crsr, handle old_under);
void cursor_list_remove(struct cursor *crsr);
void cursor_update_widget(struct widget *w, u8 new_numcursors); 

/****************************** Public cursor methods ***/

g_error cursor_new(struct cursor **crsr, handle *h, int owner) {
  g_error e;
  struct cursor *c;
  handle c_h;

  /* Allocate cursor */
  e = g_malloc((void**)&c, sizeof(struct cursor));
  errorcheck;
  memset(c,0,sizeof(struct cursor));

  /* Default at the screen center */
  cursor_getposition(NULL,&c->x, &c->y,NULL);

  /* Add to list */
  c->next = cursor_list;
  cursor_list = c;

  /* Allocate the sprite */
  e = cursor_set_theme(c, PGTH_O_DEFAULT);
  errorcheck;

  /* Give the cursor a handle. We always use pointers to
   * reference cursors internal to pgserver, but when passing
   * events to and from a client we must refer to the cursor
   * using only this handle.
   */
  e = mkhandle(&c_h, PG_TYPE_CURSOR, owner, c);
  errorcheck;

  if (crsr)
    *crsr = c;
  if (h)
    *h = c_h;

  return success;
}


void cursor_delete(struct cursor *crsr) {
  cursor_hide(crsr);
  if (crsr->sprite)
    free_sprite(crsr->sprite);
  cursor_list_remove(crsr);
  g_free(crsr);
}


void cursor_shutdown(void) {
  while (cursor_list)
    cursor_delete(cursor_list);
}


g_error cursor_set_theme(struct cursor *crsr, int thobj) {
  hwrbitmap *bitmap,*mask;
  s16 w,h;
  bool redisplay;
  g_error e;

  crsr->thobj = thobj;
  
  /* Read the cursor bitmap and bitmask. Note that if the cursor is rectangular
   * or it uses an alpha channel the mask will be NULL.
   */

  e = rdhandlep((void***)&bitmap,PG_TYPE_BITMAP,-1,theme_lookup(thobj,PGTH_P_CURSORBITMAP));
  errorcheck;

  mask = NULL;
  e = rdhandlep((void***)&mask,PG_TYPE_BITMAP,-1,theme_lookup(thobj,PGTH_P_CURSORBITMASK));
  errorcheck;

  if (crsr->sprite && bitmap==crsr->sprite->bitmap && mask==crsr->sprite->mask)
    return success;

  /* If there's no bitmap yet, it's early in init and the default cursor
   * hasn't been created yet. Skip this all.
   */
  if (!bitmap)
    return success;

  VID(bitmap_getsize) (*bitmap,&w,&h);
  
  /* Insert the new bitmaps, resize/create the sprite if necessary */

  redisplay = crsr->sprite && crsr->sprite->onscreen;
  if (redisplay)
    VID(sprite_hide) (crsr->sprite);

  /* Update the hotspot before cursor_move */
  crsr->hotspot_x = theme_lookup(thobj,PGTH_P_CRSRHOTSPOT_X);
  crsr->hotspot_y = theme_lookup(thobj,PGTH_P_CRSRHOTSPOT_Y);

  if (!crsr->sprite) {
    int x,y;
    struct divtree *dt;

    /* Get the default position */
    cursor_getposition(crsr,&x,&y,&dt);

    /* Create a new sprite (default hidden, as per description in input.h) */
    e = new_sprite(&crsr->sprite,dt,w,h);
    errorcheck;
    crsr->sprite->visible = 0;
    
    /* Set the bitmap up, and move it */
    crsr->sprite->bitmap = bitmap;
    crsr->sprite->mask = mask;
    cursor_move(crsr, x,y, dt);
  }
  else {

    /* Since we're not creating a new sprite, we might need to move the hotspot
     */
    crsr->sprite->x = crsr->x - crsr->hotspot_x;
    crsr->sprite->y = crsr->y - crsr->hotspot_y;

    if ( (w!=crsr->sprite->w) || (h!=crsr->sprite->h) ) {
      /* Resize an existing sprite 
       */
      VID(bitmap_free) (crsr->sprite->backbuffer);
      e = VID(bitmap_new) (&crsr->sprite->backbuffer,w,h,vid->bpp);
      errorcheck;
      crsr->sprite->w = w;
      crsr->sprite->h = h;
    }

    crsr->sprite->bitmap = bitmap;
    crsr->sprite->mask = mask;
  }    

  /* If the cursor has an alpha channel, set the LGOP accordingly */
  if (w && h && (VID(getpixel)(*bitmap,0,0) & PGCF_ALPHA))
    crsr->sprite->lgop = PG_LGOP_ALPHA;
  else
    crsr->sprite->lgop = PG_LGOP_NONE;
  
  if (redisplay)
    VID(sprite_show)(crsr->sprite);
  
  /* Whether it was onscreen or not, show it at the next convenient opportunity */
  crsr->sprite->visible = 1;

  return success;
}


/* This function has the job of determining what divnode the cursor
 * is hovering over, and if necessary sending enter/leave events.
 */
void cursor_move(struct cursor *crsr, int x, int y, struct divtree *dt) {
  handle old_under = crsr->ctx.widget_under;

  /* clip to the screen edge */
  if (x<0)           x = 0;
  if (y<0)           y = 0;
  if (x>=dt->head->calc.w) x = dt->head->calc.w-1;
  if (y>=dt->head->calc.h) y = dt->head->calc.h-1;

  crsr->x = x;
  crsr->y = y;
  crsr->divtree = dt->h;

  /* Move the cursor to the head of the activity list */
  if (crsr != cursor_list) {
    cursor_list_remove(crsr);

    /* Reinsert at head */
    crsr->next = cursor_list;
    cursor_list = crsr;
  }
  
  /* First update the sprite position, so that widgetunder
   * uses the new position.
   */
  crsr->sprite->x = x - crsr->hotspot_x;
  crsr->sprite->y = y - crsr->hotspot_y;

  /* Update the widget under the cursor, and if necessary,
   * send out enter/leave events.
   */
  cursor_widgetunder(crsr);
  cursor_change_under(crsr,old_under);
  
  VID(sprite_update) (crsr->sprite);
}


/* Hide/show the cursor, also keeping track of mouse entering and leaving
 */
void cursor_hide(struct cursor *crsr) {
  handle old_under = crsr->ctx.widget_under;

  if (!crsr->sprite || !crsr->sprite->visible)
    return;

  crsr->sprite->visible = 0;
  VID(sprite_hide)(crsr->sprite);
  memset(&crsr->ctx,0,sizeof(crsr->ctx));

  cursor_change_under(crsr,old_under);
}


void cursor_show(struct cursor *crsr) {
  if (crsr->sprite->visible)
    return;
  
  cursor_widgetunder(crsr);
  cursor_change_under(crsr,0);

  crsr->sprite->visible = 1;
  VID(sprite_show)(crsr->sprite);
}


/* We may put a more intelligent way of determining this here later...
 */
struct cursor *cursor_get_default(void) {
  return cursor_list;
}


/* Re-evaluate the widget under every cursor and send out events
 */
void cursor_update_hover(void) {
  handle old_under;
  struct cursor *crsr;

  for (crsr=cursor_list;crsr;crsr=crsr->next) {
    old_under = crsr->ctx.widget_under;
    cursor_widgetunder(crsr);
    cursor_change_under(crsr,old_under);
  }
}


/* Reload all cursor theme objects if necessary
 */
g_error cursor_retheme(void) {
  struct cursor *p;
  g_error e;

  for (p=cursor_list;p;p=p->next) {
    e = cursor_set_theme(p, p->thobj);
    errorcheck;
  }
   
  return success;
}

void cursor_getposition(struct cursor *crsr, int *x, int *y, struct divtree **dt) {
  if (!crsr)
    crsr = cursor_get_default();
  if (!crsr) {
    /* Start cursors out near the top-left corner */
    *x = 16;
    *y = 16;
    if (dt)
      *dt = dts->top;
  }
  else {
    *x = crsr->x;
    *y = crsr->y;

    if (dt) {
      if (iserror(rdhandle((void**)dt, PG_TYPE_DIVTREE, -1, crsr->divtree)) || !*dt)
	*dt = dts->top;
    }
  }
}


/****************************** Private cursor utility methods ***/

void cursor_widgetunder(struct cursor *crsr) {
  int x,y;
  struct divnode *div;
  struct divtree *dt;

  cursor_getposition(crsr, &x, &y, &dt);
  div = dt->head;

  /* If there are popups and they're all in the nontoolbar area,
   * we can pass toolbar's events through to the bottom layer in the dtstack
   */
  if (popup_toolbar_passthrough()) {
    struct divnode *ntb = appmgr_nontoolbar_area();
    if (ntb) {
      if (x < ntb->r.x ||
	  y < ntb->r.y ||
	  x >= ntb->r.x+ntb->r.w ||
	  y >= ntb->r.y+ntb->r.h) {
	
	/* Get a widget from the bottom layer, with the toolbars */
	div = dts->root->head;
      }
    }
  }
 
  /* recursively determine the widget/divnode under the cursor */
  crsr->ctx.div_under = NULL;
  crsr->ctx.deepest_div = NULL;
  r_cursor_widgetunder(crsr,div,x,y);

  /* Save the widget associated with the divnode we're under */
  if (crsr->ctx.div_under) {
    crsr->ctx.widget_under = hlookup(crsr->ctx.div_under->owner,NULL);

    /* Also change the cursor theme */
    cursor_set_theme(crsr, widget_get(crsr->ctx.div_under->owner,PG_WP_THOBJ));
  }
  else {
    crsr->ctx.widget_under = 0;

    /* Default cursor theme */
    cursor_set_theme(crsr, PGTH_O_DEFAULT);
  }
}

/* Recursively determine which divnode (belonging to an interactive widget)
 * is under the cursor at this time.
 */
void r_cursor_widgetunder(struct cursor *crsr, struct divnode *div,int x,int y) {

  if (!div) return;

  /* If the cursor is outside this node, 
   * ignore it and all children.
   */
  if ( (x < div->r.x) ||
       (y < div->r.y) ||
       (x > div->r.x + div->r.w - 1) ||
       (y > div->r.y + div->r.h - 1) )
    return;

  /* If the cursor is outside this node's scrolling
   * container, ignore it and all its children. 
   */
  if ( (div->flags & DIVNODE_DIVSCROLL) &&
       div->divscroll &&
       ( (x < div->divscroll->r.x) ||
	 (y < div->divscroll->r.y) ||
	 (x > div->divscroll->r.x + div->divscroll->r.w - 1) ||
	 (y > div->divscroll->r.y + div->divscroll->r.h - 1) ))
    return;

  /* If this divnode has an interactive widget as its owner, and it
   * is visible, store it in crsr->under
   */
  if (div->owner && div->owner->trigger_mask && (div->grop || div->build))
    crsr->ctx.div_under = div;
  
  /* Always store the deepest match in here */
  crsr->ctx.deepest_div = div;
  
  /* Check this divnode's children */
  r_cursor_widgetunder(crsr,div->next,x,y);
  r_cursor_widgetunder(crsr,div->div,x,y);
}


/* When the node under the cursor changes, notify the owning widget.
 */
void cursor_change_under(struct cursor *crsr, handle old_under) {
  struct widget *new_w, *old_w;

  new_w = NULL;
  rdhandle((void**)&new_w, PG_TYPE_WIDGET, -1, crsr->ctx.widget_under);
  old_w = NULL;
  rdhandle((void**)&old_w, PG_TYPE_WIDGET, -1, old_under);

  if (new_w == old_w)
    return;

  if (new_w)
    widget_set_numcursors(new_w,new_w->numcursors+1,crsr);
  if (old_w)
    widget_set_numcursors(old_w,old_w->numcursors-1,crsr);
}


void cursor_list_remove(struct cursor *crsr) {
  struct cursor **p;
  
  /* Delete it */
  for (p=&cursor_list;*p;p=&(*p)->next)
    if (*p == crsr) {
      *p = crsr->next;
      break;
    }
}


/* Change the number of mouse cursors in the widget, sending out enter/leave events */
void cursor_update_widget(struct widget *w, u8 new_numcursors) {
  if (new_numcursors && !w->numcursors)
    send_trigger(w,PG_TRIGGER_ENTER,NULL);

  if (!new_numcursors && w->numcursors)
    send_trigger(w,PG_TRIGGER_LEAVE,NULL);

  w->numcursors = new_numcursors;
}


/* The End */






