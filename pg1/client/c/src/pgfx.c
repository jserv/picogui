/* $Id$
 *
 * picogui/pgfx.c - PGFX general-purpose utility functions
 * 
 * This is a thin wrapper providing a set of primitives that can render to
 * a canvas (persistant or immediate mode) and other fast graphics interfaces
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

#include "clientlib.h"

void pgMoveTo(pgcontext c, pgu x, pgu y) {
   c->cx = x;
   c->cy = y;
}

pgprim  pgLineTo(pgcontext c, pgu x, pgu y) {
   pgLine(c,c->cx,c->cy,x,y);
   pgMoveTo(c,x,y);
}

void pgDeleteContext(pgcontext c) {
   free(c);
}

inline pgprim pgPixel(pgcontext c,pgu x,pgu y) {
   return (*(c)->lib->pixel)(c,x,y);
}
inline pgprim pgLine(pgcontext c,pgu x1,pgu y1,pgu x2,pgu y2) {
   return (*(c)->lib->line)(c,x1,y1,x2,y2);
}
inline pgprim pgRect(pgcontext c,pgu x,pgu y,pgu w,pgu h) {
   return (*(c)->lib->rect)(c,x,y,w,h);
}
inline pgprim pgBlur(pgcontext c,pgu x,pgu y,pgu w,pgu h,pgu radius) {
   return (*(c)->lib->blur)(c,x,y,w,h,radius);
}
inline pgprim pgFrame(pgcontext c,pgu x,pgu y,pgu w,pgu h) {
   return (*(c)->lib->frame)(c,x,y,w,h);
}
inline pgprim pgSlab(pgcontext c,pgu x,pgu y,pgu w) {
   return (*(c)->lib->slab)(c,x,y,w);
}
inline pgprim pgEllipse(pgcontext c,pgu x,pgu y,pgu w,pgu h) {
   return (*(c)->lib->ellipse)(c,x,y,w,h);
}
inline pgprim pgFEllipse(pgcontext c,pgu x,pgu y,pgu w,pgu h) {
   return (*(c)->lib->fellipse)(c,x,y,w,h);
}
inline pgprim pgFPolygon(pgcontext c, pghandle array) { 
   return (*(c)->lib->fpolygon)(c,array); 
} 
inline pgprim pgText(pgcontext c,pgu x,pgu y,pghandle string) {
   return (*(c)->lib->text)(c,x,y,string);
}
inline pgprim pgBitmap(pgcontext c,pgu x,pgu y,pgu w,pgu h,pghandle bitmap) {
   return (*(c)->lib->bitmap)(c,x,y,w,h,bitmap);
}
inline pgprim pgRotateBitmap(pgcontext c,pgu x,pgu y,pgu w,pgu h,pghandle bitmap) {
   return (*(c)->lib->rotatebitmap)(c,x,y,w,h,bitmap);
}
inline pgprim pgTileBitmap(pgcontext c,pgu x,pgu y,pgu w,pgu h,pghandle bitmap) {
   return (*(c)->lib->tilebitmap)(c,x,y,w,h,bitmap);
}
inline pgprim pgGradient(pgcontext c,pgu x,pgu y,pgu w,pgu h,
			 pgu angle,pgcolor c1,pgcolor c2) {
   return (*(c)->lib->gradient)(c,x,y,w,h,angle,c1,c2);
}
inline pgprim pgSetColor(pgcontext c,pgcolor color) {
   return (*(c)->lib->setcolor)(c,color);
}
inline pgprim pgSetFont(pgcontext c,pghandle font) {
   return (*(c)->lib->setfont)(c,font);
}
inline pgprim pgSetLgop(pgcontext c,short lgop) {
   return (*(c)->lib->setlgop)(c,lgop);
}
inline pgprim pgSetAngle(pgcontext c,pgu angle) {
   return (*(c)->lib->setangle)(c,angle);
}
inline pgprim pgSetSrc(pgcontext c,pgu x,pgu y,pgu w,pgu h) {
   return (*(c)->lib->setsrc)(c,x,y,w,h);
}
inline pgprim pgSetMapping(pgcontext c,pgu x,pgu y,pgu w,pgu h,short type) {
   return (*(c)->lib->setmapping)(c,x,y,w,h,type);
}
inline pgprim pgSetClip(pgcontext c,pgu x,pgu y,pgu w,pgu h) {
   return (*(c)->lib->setclip)(c,x,y,w,h);
}
inline void pgContextUpdate(pgcontext c) {
   (*(c)->lib->update)(c);
}

/* The End */
