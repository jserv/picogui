/* $Id: pgfx.c,v 1.2 2001/04/14 00:02:22 micahjd Exp $
 *
 * picogui/pgfx.c - PGFX general-purpose utility functions
 * 
 * This is a thin wrapper providing a set of primitives that can render to
 * a canvas (persistant or immediate mode) and other fast graphics interfaces
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include "clientlib.h"

void pgMoveto(pgcontext c, pgu x, pgu y) {
   c->cx = x;
   c->cy = y;
}

pgprim  pgLineto(pgcontext c, pgu x, pgu y) {
   pgLine(c,c->cx,c->cy,x,y);
   pgMoveto(c,x,y);
}

void pgDeleteContext(pgcontext c) {
   free(c);
}

/* The End */
