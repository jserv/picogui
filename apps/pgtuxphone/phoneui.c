/* $Id: phoneui.c,v 1.1 2001/10/30 01:32:28 micahjd Exp $
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

int btnRedial(struct pgEvent *evt) {
  return 0;
}

int btnHistory(struct pgEvent *evt) {
  return 0;
}

char *keypad[4][3] = {
  { "1", "2", "3" },
  { "4", "5", "6" },
  { "7", "8", "9" },
  { "*", "0", "#" }
};

/* Display an onscreen phone keypad */
int btnKeypad(struct pgEvent *evt) {
  pghandle row,button;
  int i,j;
  struct pgEvent e;
  pghandle font;

  pgEnterContext();
  pgNewPopupAt(PG_POPUP_ATCURSOR,PG_POPUP_ATCURSOR,200,300);

  font = pgNewFont(NULL,20,0);

  /* Make our button grid */
  for (i=0, row=0;i<4;i++) {
    row = pgNewWidget(PG_WIDGET_BOX,row ? PG_DERIVE_AFTER : PG_DERIVE_INSIDE,
		      row);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIZE,pgFraction(1,4),
		PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
		PG_WP_TRANSPARENT,1,
		0);
    for (j=0, button=0; j<3;j++) {
      button = pgNewWidget(PG_WIDGET_BUTTON, button ? PG_DERIVE_AFTER :
			   PG_DERIVE_INSIDE, button ? button : row);
      pgSetWidget(PGDEFAULT,
		  PG_WP_SIZE,pgFraction(1,3),
		  PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
		  PG_WP_TEXT,pgNewString(keypad[i][j]),
		  PG_WP_FONT,font,
		  0);
      pgSetPayload(PGDEFAULT,(long) keypad[i][j]);
    }
  }

  do {
    e = *pgGetEvent();
    if (e.type == PG_WE_ACTIVATE)
      phone_dial(phone_fd,pgGetString(pgGetWidget(e.from,PG_WP_TEXT)));
  } while (e.type != PG_WE_DEACTIVATE);
  pgLeaveContext();
  
  return 0;
}

/* The End */
