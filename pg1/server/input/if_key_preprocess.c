/* $Id$
 *
 * if_key_preprocess.c - Perform various processing on keys before dispatch
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

void infilter_key_preprocess_handler(struct infilter *self, u32 trigger, union trigparam *param) {

  /* Rotate key events
   */
  VID(coord_keyrotate)(&param->kbd.key);

  /* Map PG_TRIGGER_KEY to a set of CHAR, KEYUP, and KEYDOWN triggers
   */
  if (trigger == PG_TRIGGER_KEY) {
    if (param->kbd.key < 256)
      infilter_send(self, PG_TRIGGER_CHAR, param);
    infilter_send(self, PG_TRIGGER_KEYDOWN, param);
    infilter_send(self, PG_TRIGGER_KEYUP, param);
    return;
  }

  /* Don't allow CHAR events that have CTRL or ALT modifiers set
   */
  if (trigger == PG_TRIGGER_CHAR && (param->kbd.mods & (PGMOD_CTRL | PGMOD_ALT)))
    return;

  /* Pass on other events */
  infilter_send(self,trigger,param);
}

struct infilter infilter_key_preprocess = {
  /*accept_trigs:  */PG_TRIGGER_KEY | PG_TRIGGER_CHAR | PG_TRIGGER_KEYUP | PG_TRIGGER_KEYDOWN,
  /*absorb_trigs:  */PG_TRIGGER_KEY | PG_TRIGGER_CHAR | PG_TRIGGER_KEYUP | PG_TRIGGER_KEYDOWN,
       /*handler:  */&infilter_key_preprocess_handler
};

/* The End */






