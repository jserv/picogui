/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 *
 *  Seriously modified by Fredrik Hübinette <hubbe@hubbe.net>
 *
 *  Hacked up some more by Micah Dowty <micahjd@users.sourceforge.net>
 *
 */

/*
 * x2pgui - control PicoGUI servers in a multihead style
 */

#include "x2pgui.h"
#include <X11/keysym.h>
#include <X11/Xutil.h>

struct pgmodeinfo mi;
static pghandle cursor = 0;

int main(int argc, char **argv)
{
  fd_set fds;
  struct timeval tv, *tvp;
  int msWait;

  pgInit(argc,argv);
  processArgs(argc, argv);
  
  mi = *pgGetVideoMode();
 
  if (!CreateXWindow()) exit(1);
  HandleXEvents();

  return 0;
}

void HidePointer(void) {
  if (cursor)
    pgDelete(cursor);
  cursor = 0;
  pgUpdate();
}

Bool SendPointerEvent(int x, int y, int buttonMask) {
  static union pg_client_trigger trig;

  if (!cursor)
    cursor = pgNewCursor();

  trig.content.type = PG_TRIGGER_PNTR_STATUS;
  trig.content.u.mouse.x = x;
  trig.content.u.mouse.y = y;
  trig.content.u.mouse.btn = buttonMask;
  trig.content.u.mouse.cursor_handle = cursor;
  trig.content.u.mouse.is_logical = 1;

  pgInFilterSend(&trig);
  pgFlushRequests();
  return 1;
}

