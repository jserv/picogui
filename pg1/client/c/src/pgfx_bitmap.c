/* $Id$
 *
 * picogui/pgfx_bitmap.c - lib functions and registration for offscreen bitmap
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

pgprim _pgfxbitmap_pixel(pgcontext c, pgu x,  pgu y) {
   pgRender(c->device,PG_GROP_PIXEL,x,y,1,1);
   return 0;
}

pgprim _pgfxbitmap_line(pgcontext c, pgu x1, pgu y1, pgu x2, pgu y2) {
   pgRender(c->device,PG_GROP_LINE,x1,y1,x2-x1,y2-y1);
   return 0;
}

pgprim _pgfxbitmap_rect(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h) {
   pgRender(c->device,PG_GROP_RECT,x,y,w,h);
   return 0;
}

pgprim _pgfxbitmap_blur(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,  pgu radius) {
   pgRender(c->device,PG_GROP_BLUR,x,y,w,h,radius);
   return 0;
}

pgprim _pgfxbitmap_frame(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h) {
   pgRender(c->device,PG_GROP_FRAME,x,y,w,h);
   return 0;
}

pgprim _pgfxbitmap_slab(pgcontext c, pgu x,  pgu y,  pgu w) {
   pgRender(c->device,PG_GROP_SLAB,x,y,w,1);
   return 0;
}

pgprim _pgfxbitmap_bar(pgcontext c, pgu x,  pgu y,  pgu h) {
   pgRender(c->device,PG_GROP_BAR,x,y,1,h);
   return 0;
}

pgprim _pgfxbitmap_ellipse(pgcontext c, pgu x,  pgu y, pgu w,  pgu h) { 
   pgRender(c->device,PG_GROP_ELLIPSE,x,y,w,h); 
   return 0; 
} 
 
pgprim _pgfxbitmap_fellipse(pgcontext c, pgu x,  pgu y, pgu w,  pgu h) { 
   pgRender(c->device,PG_GROP_FELLIPSE,x,y,w,h); 
   return 0; 
} 
 
pgprim _pgfxbitmap_fpolygon(pgcontext c, pghandle array) { 
   pgRender(c->device,PG_GROP_FPOLYGON,1,1,1,1,array); 
	return 0;
} 
 
pgprim _pgfxbitmap_text(pgcontext c, pgu x,  pgu y,  pghandle string) {
   pgRender(c->device,PG_GROP_TEXT,x,y,1,1,string);
   return 0;
}

pgprim _pgfxbitmap_bitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			  pghandle bitmap) {
   pgRender(c->device,PG_GROP_BITMAP,x,y,w,h,bitmap);
   return 0;
}

pgprim _pgfxbitmap_rotatebitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
				pghandle bitmap) {
   pgRender(c->device,PG_GROP_ROTATEBITMAP,x,y,w,h,bitmap);
   return 0;
}

pgprim _pgfxbitmap_tilebitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			      pghandle bitmap) {
   pgRender(c->device,PG_GROP_TILEBITMAP,x,y,w,h,bitmap);
   return 0;
}

pgprim _pgfxbitmap_gradient(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			    pgu angle, pgcolor c1, pgcolor c2) {
   pgRender(c->device,PG_GROP_GRADIENT,x,y,w,h,angle,c1,c2);
   return 0;
}

pgprim _pgfxbitmap_setcolor(pgcontext c, pgcolor color) {
   pgRender(c->device,PG_GROP_SETCOLOR,color);
   return 0;
}

pgprim _pgfxbitmap_setfont(pgcontext c, pghandle font) {
   pgRender(c->device,PG_GROP_SETFONT,font);
   return 0;
}

pgprim _pgfxbitmap_setlgop(pgcontext c, short lgop) {
   pgRender(c->device,PG_GROP_SETLGOP,lgop);
   return 0;
}

pgprim _pgfxbitmap_setangle(pgcontext c, pgu angle) {
   pgRender(c->device,PG_GROP_SETANGLE,angle);
   return 0;
}

pgprim _pgfxbitmap_setsrc(pgcontext c, pgu x,pgu y,pgu w,pgu h) {
   pgRender(c->device,PG_GROP_SETSRC,x,y,w,h);
   return 0;
}

pgprim _pgfxbitmap_setmapping(pgcontext c, 
			      pgu x,pgu y,pgu w,pgu h,short type) {
   pgRender(c->device,PG_GROP_SETMAPPING,x,y,w,h,type);
   return 0;
}

pgprim _pgfxbitmap_setclip(pgcontext c, 
			   pgu x,pgu y,pgu w,pgu h) {
   pgRender(c->device,PG_GROP_SETCLIP,x,y,w,h);
   return 0;
}

void _pgfxbitmap_update(pgcontext c) {
	/* Normally an update is not necessary, but if we're rendering
	 * directly to the display there might be double-buffering
	 * that requires updating. The app can get fine-grained control
	 * over this by using PG_GROP_VIDUPDATE directly, but this is the
	 * easy way.
	 * 
	 * We don't necessarily know the screen size here, so just
	 * give it a ludicrously high value and let PicoGUI clip it.
	 */
	if (!c->device)
      pgRender(0,PG_GROP_VIDUPDATE,0,0,20000,20000);
}

/******************************* Registration */

pgcontext pgNewBitmapContext(pghandle bitmap) {
   static struct pgfx_lib l;  /* Should only need one of these */
   pgcontext ctx;
   
   ctx = _pg_malloc(sizeof(struct pgfx_context));
   if (!ctx) return NULL;
   memset(ctx,0,sizeof(struct pgfx_context));

   l.pixel        = _pgfxbitmap_pixel;
   l.line         = _pgfxbitmap_line;
   l.rect         = _pgfxbitmap_rect;
   l.frame        = _pgfxbitmap_frame;
   l.blur         = _pgfxbitmap_blur;
   l.slab         = _pgfxbitmap_slab;
   l.bar          = _pgfxbitmap_bar;
   l.ellipse      = _pgfxbitmap_ellipse; 
   l.fellipse     = _pgfxbitmap_fellipse; 
   l.fpolygon     = _pgfxbitmap_fpolygon; 
   l.text         = _pgfxbitmap_text;
   l.bitmap       = _pgfxbitmap_bitmap;
   l.rotatebitmap = _pgfxbitmap_rotatebitmap;
   l.tilebitmap   = _pgfxbitmap_tilebitmap;
   l.gradient     = _pgfxbitmap_gradient;
   l.setcolor     = _pgfxbitmap_setcolor;
   l.setfont      = _pgfxbitmap_setfont;
   l.setlgop      = _pgfxbitmap_setlgop;
   l.setangle     = _pgfxbitmap_setangle;
   l.setsrc       = _pgfxbitmap_setsrc;
   l.setmapping   = _pgfxbitmap_setmapping;
   l.setclip      = _pgfxbitmap_setclip;
   l.update       = _pgfxbitmap_update;
   
   ctx->lib = &l;
   ctx->device = bitmap;

   return ctx;
}

/* The End */
