/* $Id: if_pntr_dispatch.c,v 1.4 2002/10/02 21:05:21 micahjd Exp $
 *
 * if_pntr_dispatch.c - Dispatch mouse pointer events to widgets
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

void infilter_pntr_dispatch_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  struct widget *under;
  int release_captured = 0;

  /* In order to dispatch a pointing event, it must have an associated cursor.
   * The normalize filter should have given us a cursor whether the driver had
   * one or not, but in case an event was inserted into the pipe with no cursor,
   * pass it on. This will usually just run the event off the end of the
   * filter chain, but this makes it theoretically possible for a client to pick
   * up the cursorless events.
   */
  if (!param->mouse.cursor) {
    infilter_send(self, trigger, param);
    return;
  }

  /* Move the cursor */
  if (trigger==PG_TRIGGER_MOVE) {
    int cx,cy;

    /* If the cursor is already at the destination, throw away this event */
    cursor_getposition(param->mouse.cursor,&cx,&cy);
    if (cx == param->mouse.x && cy == param->mouse.y)
      return;

    cursor_move(param->mouse.cursor,param->mouse.x,param->mouse.y);
  }

  inactivity_reset();

  /* Read which widget is under the cursor */
  under = NULL;
  rdhandle((void**)&under, PG_TYPE_WIDGET, -1, param->mouse.cursor->ctx.widget_under);

  /* If the capture_btn is released, release the capture 
   */
  if ((!(param->mouse.btn & param->mouse.cursor->ctx.capture_btn)) && 
      param->mouse.cursor->ctx.widget_capture && 
      param->mouse.cursor->ctx.widget_capture != param->mouse.cursor->ctx.widget_under) {
    struct widget *capture;
    
    if ((!iserror(rdhandle((void**)&capture, PG_TYPE_WIDGET, -1,
			   param->mouse.cursor->ctx.widget_capture))) && capture)
      release_captured = send_trigger(capture,PG_TRIGGER_RELEASE,param);
    
    param->mouse.cursor->ctx.widget_capture = 0;
    param->mouse.cursor->ctx.capture_btn = 0;
  }

  if (under) {
    /* There's an interactive widget under the cursor */
  
    /* Keep track of the most recently clicked widget 
     */
    if (trigger==PG_TRIGGER_DOWN) {
      param->mouse.cursor->ctx.widget_last_clicked = param->mouse.cursor->ctx.widget_under;
      
      /* Also, allow clicks to focus applications */
      appmgr_focus(appmgr_findapp(under));
    }

    /* First send the 'raw' event, then handle the cooked ones. */
    if (!release_captured)
      send_propagating_trigger(under,trigger,param);
    
    /* If the mouse is clicked in a widget, it 'captures' future MOVE and RELEASE events
     * until this button is released.
     */
    if (trigger==PG_TRIGGER_DOWN && !param->mouse.cursor->ctx.widget_capture) {
      param->mouse.cursor->ctx.widget_capture = param->mouse.cursor->ctx.widget_under;
      param->mouse.cursor->ctx.capture_btn = param->mouse.chbtn;
    }
  }

  /* If a captured widget accepts PG_TRIGGER_DRAG, send it even when the
   * mouse is outside its divnodes. 
   */
  if (trigger==PG_TRIGGER_MOVE && param->mouse.cursor->ctx.widget_capture) {
    struct widget *capture;
    
    if ((!iserror(rdhandle((void**)&capture, PG_TYPE_WIDGET, -1,
			   param->mouse.cursor->ctx.widget_capture))) && capture)
      send_trigger(capture,PG_TRIGGER_DRAG,param);
  }
}

struct infilter infilter_pntr_dispatch = {
  accept_trigs: PG_TRIGGER_MOVE | PG_TRIGGER_UP | PG_TRIGGER_DOWN | PG_TRIGGER_SCROLLWHEEL,
  absorb_trigs: PG_TRIGGER_MOVE | PG_TRIGGER_UP | PG_TRIGGER_DOWN,
  handler: &infilter_pntr_dispatch_handler,
};

/* The End */






