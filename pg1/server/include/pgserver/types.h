/* $Id$
 *
 * pgserver/types.h - Common types used throughout pgserver
 *                    (but only in pgserver)
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

#ifndef __SERVER_TYPES_H
#define __SERVER_TYPES_H

typedef short int alignt;

struct fraction {
  int n,d;
};

struct pgpair {
  s16 x,y;
};

struct sizepair {
  s16 w,h;
};

struct fractionpair {
  struct fraction x,y;
};

struct pgquad {
  s16 x1,y1,x2,y2;
};

struct pgrect {
  s16 x,y,w,h;
};

#endif /* __SERVER_TYPES_H */

/* The End */
