/* $Id$
 *
 * phoneui.c - UI functions other than the basic phonecall info bar
 *
 * Copyright (C) 2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#include <picogui.h>
#include "phoneui.h"
#include "tuxclient.h"
#include "phonecall.h"

pghandle wKeypad;

int btnRedial(struct pgEvent *evt) {
  phone_dial(phone_fd,redial_number);
  return 0;
}

char *keypad[4][3] = {
  { " 1 ", " 2 ", " 3 " },
  { " 4 ", " 5 ", " 6 " },
  { " 7 ", " 8 ", " 9 " },
  { " * ", " 0 ", " # " }
};

/* Event handler for keypad dial buttons. Extra is a pointer
 * to the single character digit to dial */
int btnDial(struct pgEvent *evt) {
  char c[2] = " ";
  c[0] = *(char *)evt->extra;

  phone_dial(phone_fd,c);
  return 0;
}

/* Turn the keypad on/off */
int btnKeypad(struct pgEvent *evt) {
  pgSetWidget(wKeypad,
	      PG_WP_SIZE, pgGetWidget(evt->from,PG_WP_ON) ? -1 : 0,
	      0);
  return 0;
}

/* Create the phone keypad so we can easily turn it on and off later */
void init_keypad(void) {
  pghandle column,button;
  int i,j;
  struct pgEvent e;
  pghandle font;

  /* Create the keypad toolbar, invisible for now */
  wKeypad = pgRegisterApp(PG_APP_TOOLBAR,"pgtuxphone/keypad",
			  PG_APPSPEC_SIDE, PG_S_BOTTOM,
			  0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,0,
	      0);

  font = pgNewFont(NULL,20,0);

  /* Make our button grid */
  for (i=0, column=0;i<3;i++) {
    column = pgNewWidget(PG_WIDGET_BOX,column ? PG_DERIVE_AFTER : PG_DERIVE_INSIDE,
			 column);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,1,
		PG_WP_SIDE,PG_S_LEFT,
		0);
    for (j=0, button=0; j<4;j++) {
      button = pgNewWidget(PG_WIDGET_BUTTON, button ? PG_DERIVE_AFTER :
			   PG_DERIVE_INSIDE, button ? button : column);
      pgSetWidget(PGDEFAULT,
		  //		  PG_WP_SIZE,pgFraction(1,4),
		  //		  PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
		  PG_WP_TEXT,pgNewString(keypad[j][i]),
		  PG_WP_FONT,font,
		  PG_WP_SIDE,PG_S_TOP,
		  0);

      /* Extra is a pointer to the digit to dial. Since there's some padding
       * in the button strings, the +1 gives the character itself
       */
      pgBind(PGDEFAULT,PG_WE_ACTIVATE,btnDial,keypad[j][i]+1);
    }
  }
}

/* The End */
