/* $Id: cursor.c,v 1.1 2002/05/24 05:43:19 micahjd Exp $
 *
 * cursor.c - Cursor abstraction and multiplexing layer 
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>

/* List of all cursors */
struct cursor *cursor_list;

void r_cursor_widgetunder(struct cursor *crsr, struct divnode *div);
void cursor_change_under(struct cursor *crsr, struct divnode *old_under);
void cursor_list_remove(struct cursor *crsr);
void cursor_update_widget(struct widget *w, u8 new_numcursors); 

/****************************** Public cursor methods ***/

g_error cursor_new(struct cursor **crsr) {
  g_error e;

  /* Allocate cursor */
  e = g_malloc((void**)crsr, sizeof(struct cursor));
  errorcheck;
  memset(*crsr,0,sizeof(struct cursor));

  /* We save the sprite allocation until later when we set the theme... */

  /* Add to list */
  (*crsr)->next = cursor_list;
  cursor_list = *crsr;

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
  
  /* Read the cursor bitmap and bitmask. Note that if the cursor is rectangular
   * or it uses an alpha channel the mask will be NULL.
   */

  e = rdhandlep((void***)&bitmap,PG_TYPE_BITMAP,-1,theme_lookup(thobj,PGTH_P_CURSORBITMAP));
  errorcheck;

  mask = NULL;
  e = rdhandlep((void***)&mask,PG_TYPE_BITMAP,-1,theme_lookup(thobj,PGTH_P_CURSORBITMASK));
  errorcheck;

  VID(bitmap_getsize) (*bitmap,&w,&h);
  
  /* Insert the new bitmaps, resize/create the sprite if necessary */

  redisplay = crsr->sprite && crsr->sprite->onscreen;
  if (redisplay)
    VID(sprite_hide) (crsr->sprite);

  if (!crsr->sprite) {
    int x,y;

    /* Create a new sprite (default hidden, as per description in input.h) */

    e = new_sprite(&crsr->sprite,w,h);
    errorcheck;
    VID(sprite_hide) (crsr->sprite);

    /* Also move it to the default position 
     * (returned by cursor_getposition when there's no sprite yet) 
     */
    cursor_getposition(crsr,&x,&y);
    cursor_move(crsr, x,y);
  }
  else {
    int new_hotspot_x, new_hotspot_y;

    /* Since we're not creating a new sprite, we might need to move the hotspot
     */
    new_hotspot_x = theme_lookup(thobj,PGTH_P_CRSRHOTSPOT_X);
    new_hotspot_y = theme_lookup(thobj,PGTH_P_CRSRHOTSPOT_Y);
    crsr->sprite->x += crsr->hotspot_x - new_hotspot_x;
    crsr->sprite->y += crsr->hotspot_y - new_hotspot_y;

    if ( (w!=crsr->sprite->w) || (h!=crsr->sprite->h) ) {
      /* Resize an existing sprite 
       */
      VID(bitmap_free) (crsr->sprite->backbuffer);
      e = VID(bitmap_new) (&crsr->sprite->backbuffer,w,h,vid->bpp);
      errorcheck;
      crsr->sprite->w = w;
      crsr->sprite->h = h;
    }
  }    

  crsr->sprite->bitmap = bitmap;
  crsr->sprite->mask = mask;
  
  /* If the cursor has an alpha channel, set the LGOP accordingly */
  if (w && h && (VID(getpixel)(*bitmap,0,0) & PGCF_ALPHA))
    crsr->sprite->lgop = PG_LGOP_ALPHA;
  else
    crsr->sprite->lgop = PG_LGOP_NONE;
  
  if (redisplay)
    VID(sprite_show)(crsr->sprite);
}


/* This function has the job of determining what divnode the cursor
 * is hovering over, and if necessary sending enter/leave events.
 */
void cursor_move(struct cursor *crsr, int x, int y) {
  struct divnode *old_under = crsr->under;
  if (!crsr->sprite)
    return;

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

  if (crsr->sprite->visible) {
    /* Update the widget under the cursor, and if necessary,
     * send out enter/leave events.
     */
    r_cursor_widgetunder(crsr,dts->top->head);
    cursor_change_under(crsr,old_under);
    
    VID(sprite_update) (crsr->sprite);
  }
}


void cursor_getposition(struct cursor *crsr, int *x, int *y) {
  /* If crsr is NULL, get the default sprite */
  if (!crsr)
    crsr = cursor_get_default();

  /* If there's no sprite yet, return the default position 
   */
  if (!crsr || !crsr->sprite) {
    *x = vid->lxres >> 1;
    *y = vid->lyres >> 1;
    return;
  }

  *x = crsr->sprite->x + crsr->hotspot_x;
  *y = crsr->sprite->y + crsr->hotspot_y;
}


/* Hide/show the cursor, also keeping track of mouse entering and leaving
 */
void cursor_hide(struct cursor *crsr) {
  struct divnode *old_under = crsr->under;

  if ((!crsr->sprite) || (!crsr->sprite->visible))
    return;

  VID(sprite_hide)(crsr->sprite);
  crsr->under = NULL;
  crsr->deepest_div = NULL;

  cursor_change_under(crsr,old_under);
}


void cursor_show(struct cursor *crsr) {
  if ((!crsr->sprite) || crsr->sprite->visible)
    return;
  
  r_cursor_widgetunder(crsr,dts->top->head);
  cursor_change_under(crsr,NULL);  

  VID(sprite_show)(crsr->sprite);
}


/* We may put a more intelligent way of determining this here later...
 */
struct cursor *cursor_get_default(void) {
  return cursor_list;
}


/****************************** Private cursor utility methods ***/


/* Recursively determine which divnode (belonging to an interactive widget)
 * is under the cursor at this time.
 */
void r_cursor_widgetunder(struct cursor *crsr, struct divnode *div) {
  int x,y;

  if (!div) return;

  /* Check whether the cursor is in this divnode...
   * this is made complex by scrolling, if it is in use.
   */
  cursor_getposition(crsr, &x, &y);
  if ( ((!((div->flags & DIVNODE_DIVSCROLL) && div->divscroll)) ||
	( div->divscroll->calcx<=x && div->divscroll->calcy<=y &&
	  (div->divscroll->calcx+div->divscroll->calcw)>x &&
	  (div->divscroll->calcy+div->divscroll->calch)>y )) &&
       div->x<=x && div->y<=y && (div->x+div->w)>x && (div->y+div->h)>y) {

    /* The cursor is inside this divnode */
    
    /* If this divnode has an interactive widget as its owner, and it
     * is visible, store it in crsr->under
     */
    if (div->owner && div->owner->trigger_mask && (div->grop || div->build))
      crsr->under = div;
    
    /* Always store the deepest match in here */
    crsr->deepest_div = div;

    /* Check this divnode's children */
    r_cursor_widgetunder(crsr,div->next);
    r_cursor_widgetunder(crsr,div->div);
  }
}


/* When the node under the cursor changes, notify the owning widget.
 */
void cursor_change_under(struct cursor *crsr, struct divnode *old_under) {
  if (crsr->under != old_under)
    return;
  if (crsr->under)
    widget_set_numcursors(crsr->under->owner,crsr->under->owner->numcursors+1);
  if (old_under)
    widget_set_numcursors(old_under->owner,old_under->owner->numcursors-1);
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






