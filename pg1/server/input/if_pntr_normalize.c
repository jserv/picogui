/* $Id$
 *
 * if_pntr_normalize.c - Convert the various pointer events to a standard form
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

void infilter_pntr_normalize_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  int x,y, oldbtn, newbtn;
  static struct cursor *cursor_global_invisible;

  /* If we have pointer events with no cursor, assign them our invisible global
   * cursor so that it can collect context information that we'll need for dispatch.
   */
  if (!param->mouse.cursor) {
    if (!cursor_global_invisible) {
      if (iserror(cursor_new(&cursor_global_invisible,NULL,-1)))
	return;
      cursor_global_invisible->sprite->visible = 0;
    }
    param->mouse.cursor = cursor_global_invisible;
  }

  if (trigger != PG_TRIGGER_SCROLLWHEEL) {
    
    /* Get physical cursor position for use later */
    cursor_getposition(param->mouse.cursor, &x, &y,NULL);
    if (!param->mouse.is_logical) {
      /* Normal rotation handling */
      VID(coord_physicalize)(&x,&y);
    }
    
    /* Convert relative motion to absolute motion.
     * We have to be careful about which coordinate system the input is in.
     */
    if (trigger==PG_TRIGGER_PNTR_RELATIVE) {
      trigger = PG_TRIGGER_PNTR_STATUS;
      param->mouse.x += x;
      param->mouse.y += y;
      if (param->mouse.is_logical)
	VID(coord_physicalize)(&x,&y);
      param->mouse.is_logical = 0;
    }
    
    /* Convert absolute motion to individual events
     */
    if (trigger==PG_TRIGGER_PNTR_STATUS) {
      /* Save the old/new button state too, since it may be modified by other
       * input filters when we do infilter_send. Remember that this is all working
       * with the same trigparam structure, and just repassing it after changing
       * the type :)
       */
      newbtn = param->mouse.btn;
      oldbtn = param->mouse.cursor->prev_buttons;
      
      if (newbtn & ~oldbtn)
	trigger = PG_TRIGGER_DOWN;
      else if (oldbtn & ~newbtn)
	trigger = PG_TRIGGER_UP;
      else
	trigger = PG_TRIGGER_MOVE;
    }
    
    /* If we're moving the cursor and this isn't a MOVE event, generate one
     */
    if ((param->mouse.x != x || param->mouse.y != y) && trigger!=PG_TRIGGER_MOVE) {
      union trigparam moveparam = *param;
      moveparam.mouse.btn = param->mouse.cursor->prev_buttons;
      infilter_send(self,PG_TRIGGER_MOVE,&moveparam);
    }
  }    

  /* Detect changes in buttons, and store that along with the event
   */
  param->mouse.chbtn = param->mouse.btn ^ param->mouse.cursor->prev_buttons;
  param->mouse.cursor->prev_buttons = param->mouse.btn;

  /* Resend it (we might have changed the trigger type) */
  infilter_send(self,trigger,param);
}

struct infilter infilter_pntr_normalize = {
  /*accept_trigs:  */PG_TRIGGER_UP | PG_TRIGGER_DOWN | PG_TRIGGER_MOVE | PG_TRIGGER_PNTR_STATUS |
                     PG_TRIGGER_PNTR_RELATIVE | PG_TRIGGER_SCROLLWHEEL,
  /*absorb_trigs:  */PG_TRIGGER_UP | PG_TRIGGER_DOWN | PG_TRIGGER_MOVE | PG_TRIGGER_PNTR_STATUS |
                     PG_TRIGGER_PNTR_RELATIVE | PG_TRIGGER_SCROLLWHEEL,
       /*handler:  */&infilter_pntr_normalize_handler
};

/* The End */






