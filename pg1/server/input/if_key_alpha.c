/* $Id$
 *
 * if_key_alpha.c - Process the "Alpha" key, used to enter text on numeric keypads
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
#include <string.h>

/*************************************************** Input filter ****/

void infilter_key_alpha_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  static int last_char_key;
  union trigparam newparam;

  /* Save the key for the last PG_TRIGGER_CHAR event recieved
   */
  if (trigger == PG_TRIGGER_CHAR) {
    last_char_key = param->kbd.key;
    return;
  }

  if (param->kbd.key == PGKEY_ALPHA) {

    /* Nifty table for looking up the next alpha character. To find the
     * next character in the rotation, look up the first occurance of the
     * existing character, and the next character in the string is the
     * character to replace it with.
     */
    static const char alphatab[] = 
      "1qzQZ12abcABC23defDEF3"
      "4ghiGHI45jklJKL56mnoMNO6"
      "7prsPRS78tuvTUV89wxyWXY90 $.#0";
    const char *p;
    
    if (last_char_key > 255)
      return;
    p = strchr(alphatab,last_char_key);
    if (!p)
      return;
    p++;
    
    /* The new character is in '*p' now. Simulate a backspace and the
     * new character.
     */
    
    memset(&newparam,0,sizeof(newparam));
    newparam.kbd.key = PGKEY_BACKSPACE;
    infilter_send(self,PG_TRIGGER_CHAR,&newparam);
    
    memset(&newparam,0,sizeof(newparam));
    newparam.kbd.key = *p;
    infilter_send(self,PG_TRIGGER_CHAR,&newparam);

    last_char_key = *p;
  }
}

struct infilter infilter_key_alpha = {
  accept_trigs: PG_TRIGGER_KEYDOWN | PG_TRIGGER_CHAR,
  absorb_trigs: 0,
  handler: &infilter_key_alpha_handler,
};

/* The End */

