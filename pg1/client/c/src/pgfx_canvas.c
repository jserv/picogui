/* $Id$
 *
 * picogui/pgfx_canvas.c - lib functions and registration for canvas
 *                         drawing through the PGFX interface
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

/******************************* Generic primitives */

pgprim _pgfxcanvas_pixel(pgcontext c, pgu x,  pgu y) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_PIXEL,x,y,1,1);
   return 0;
}

pgprim _pgfxcanvas_line(pgcontext c, pgu x1, pgu y1, pgu x2, pgu y2) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_LINE,x1,y1,x2-x1,y2-y1);
   return 0;
}

pgprim _pgfxcanvas_rect(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_RECT,x,y,w,h);
   return 0;
}

pgprim _pgfxcanvas_blur(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h, pgu radius) {
   pgWriteCmd(c->device,PGCANVAS_GROP,6,PG_GROP_BLUR,x,y,w,h,radius);
   return 0;
}

pgprim _pgfxcanvas_frame(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_FRAME,x,y,w,h);
   return 0;
}

pgprim _pgfxcanvas_slab(pgcontext c, pgu x,  pgu y,  pgu w) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_SLAB,x,y,w,1);
   return 0;
}

pgprim _pgfxcanvas_bar(pgcontext c, pgu x,  pgu y,  pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_BAR,x,y,1,h);
   return 0;
}

pgprim _pgfxcanvas_ellipse(pgcontext c, pgu x,  pgu y, pgu w,  pgu h) { 
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_ELLIPSE,x,y,w,h); 
   return 0; 
} 
 
pgprim _pgfxcanvas_fellipse(pgcontext c, pgu x,  pgu y, pgu w,  pgu h) { 
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_FELLIPSE,x,y,w,h); 
   return 0; 
} 
 
pgprim _pgfxcanvas_fpolygon(pgcontext c, pghandle array) { 
  pgWriteCmd(c->device,PGCANVAS_GROP,6,PG_GROP_FPOLYGON,1,1,1,1,array); 
} 
 
pgprim _pgfxcanvas_text(pgcontext c, pgu x,  pgu y,  pghandle string) {
   pgWriteCmd(c->device,PGCANVAS_GROP,6,PG_GROP_TEXT,x,y,1,1,string);
   return 0;
}

pgprim _pgfxcanvas_bitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			  pghandle bitmap) {
   pgWriteCmd(c->device,PGCANVAS_GROP,6,PG_GROP_BITMAP,x,y,w,h,bitmap);
   return 0;
}

pgprim _pgfxcanvas_rotatebitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
				pghandle bitmap) {
   pgWriteCmd(c->device,PGCANVAS_GROP,6,PG_GROP_ROTATEBITMAP,x,y,w,h,bitmap);
   return 0;
}

pgprim _pgfxcanvas_tilebitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			      pghandle bitmap) {
   pgWriteCmd(c->device,PGCANVAS_GROP,6,PG_GROP_TILEBITMAP,x,y,w,h,bitmap);
   return 0;
}

pgprim _pgfxcanvas_gradient(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			    pgu angle, pgcolor c1, pgcolor c2) {
   pgWriteCmd(c->device,PGCANVAS_GROP,8,PG_GROP_GRADIENT,
	      x,y,w,h,angle,c1,c2);
   return 0;
}

pgprim _pgfxcanvas_setcolor(pgcontext c, pgcolor color) {
   pgWriteCmd(c->device,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,color);
   return 0;
}

pgprim _pgfxcanvas_setfont(pgcontext c, pghandle font) {
   pgWriteCmd(c->device,PGCANVAS_GROP,2,PG_GROP_SETFONT,font);
   return 0;
}

pgprim _pgfxcanvas_setlgop(pgcontext c, short lgop) {
   pgWriteCmd(c->device,PGCANVAS_GROP,2,PG_GROP_SETLGOP,lgop);
   return 0;
}

pgprim _pgfxcanvas_setangle(pgcontext c, pgu angle) {
   pgWriteCmd(c->device,PGCANVAS_GROP,2,PG_GROP_SETANGLE,angle);
   return 0;
}

pgprim _pgfxcanvas_setsrc(pgcontext c, pgu x,pgu y,pgu w,pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_SETSRC,x,y,w,h);
   return 0;
}

pgprim _pgfxcanvas_setmapping(pgcontext c, 
			      pgu x,pgu y,pgu w,pgu h,short type) {
   pgWriteCmd(c->device,PGCANVAS_GROP,6,PG_GROP_SETMAPPING,x,y,w,h,type);
   pgWriteCmd(c->device,PGCANVAS_INPUTMAPPING,5,x,y,w,h,type);
   return 0;
}

pgprim _pgfxcanvas_setclip(pgcontext c, 
			   pgu x,pgu y,pgu w,pgu h) {
  pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_SETCLIP,x,y,w,h);
  return 0;
}

void _pgfxcanvas_update(pgcontext c) {
   if (c->flags == PGFX_IMMEDIATE)
     pgWriteCmd(c->device,PGCANVAS_INCREMENTAL,0);
   else
     pgWriteCmd(c->device,PGCANVAS_REDRAW,0);

   pgSubUpdate(c->device);
}

/******************************* Registration */

pgcontext pgNewCanvasContext(pghandle canvas,short mode) {
   static struct pgfx_lib l;  /* Should only need one of these */
   pgcontext ctx;
   
   ctx = _pg_malloc(sizeof(struct pgfx_context));
   if (!ctx) return NULL;
   memset(ctx,0,sizeof(struct pgfx_context));

   l.pixel        = _pgfxcanvas_pixel;
   l.line         = _pgfxcanvas_line;
   l.rect         = _pgfxcanvas_rect;
   l.blur         = _pgfxcanvas_blur;
   l.frame        = _pgfxcanvas_frame;
   l.slab         = _pgfxcanvas_slab;
   l.bar          = _pgfxcanvas_bar;
   l.ellipse      = _pgfxcanvas_ellipse; 
   l.fellipse     = _pgfxcanvas_fellipse; 
   l.fpolygon     = _pgfxcanvas_fpolygon; 
   l.text         = _pgfxcanvas_text;
   l.bitmap       = _pgfxcanvas_bitmap;
   l.rotatebitmap = _pgfxcanvas_rotatebitmap;
   l.tilebitmap   = _pgfxcanvas_tilebitmap;
   l.gradient     = _pgfxcanvas_gradient;
   l.setcolor     = _pgfxcanvas_setcolor;
   l.setfont      = _pgfxcanvas_setfont;
   l.setlgop      = _pgfxcanvas_setlgop;
   l.setangle     = _pgfxcanvas_setangle;
   l.setsrc       = _pgfxcanvas_setsrc;
   l.setmapping   = _pgfxcanvas_setmapping;
   l.setclip      = _pgfxcanvas_setclip;
   l.update       = _pgfxcanvas_update;
   
   ctx->lib = &l;
   ctx->device = canvas ? canvas : _pgdefault_widget;
   ctx->flags = mode;
   
   if (mode == PGFX_IMMEDIATE)
     pgWriteCmd(ctx->device,PGCANVAS_DEFAULTFLAGS,1,PG_GROPF_TRANSIENT);
   
   return ctx;
}

/* The End */
