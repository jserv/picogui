/* $Id: pgfx_canvas.c,v 1.4 2001/04/14 22:47:55 micahjd Exp $
 *
 * picogui/pgfx_canvas.c - lib functions and registration for canvas
 *                         drawing through the PGFX interface
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

/******************************* Utilities */

/* Keep track of gropnode sequence and manage flags */
pgprim _pgfxcanvas_postprocess(pgcontext c) {
   if (c->flags == PGFX_IMMEDIATE)
     pgWriteCmd(c->device,PGCANVAS_GROPFLAGS,1,PG_GROPF_TRANSIENT);
     
   return c->sequence++;
}

/******************************* Generic primitives */

pgprim _pgfxcanvas_pixel(pgcontext c, pgu x,  pgu y) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_PIXEL,x,y,1,1);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,1,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,1);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_line(pgcontext c, pgu x1, pgu y1, pgu x2, pgu y2) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_LINE,x1,y1,x2-x1,y2-y1);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,1,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,1);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_rect(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_RECT,x,y,w,h);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,1,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,1);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_dim(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_DIM,x,y,w,h);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_frame(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_FRAME,x,y,w,h);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,1,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,1);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_slab(pgcontext c, pgu x,  pgu y,  pgu w) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_SLAB,x,y,w,1);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,1,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,1);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_bar(pgcontext c, pgu x,  pgu y,  pgu h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_BAR,x,y,1,h);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,1,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,1);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_text(pgcontext c, pgu x,  pgu y,  pghandle string,
			pghandle font) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_TEXT,x,y,1,1);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,3,string,font,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,4);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_textv(pgcontext c, pgu x,  pgu y,  pghandle string,
			 pghandle font) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_TEXTV,x,y,1,1);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,3,string,font,c->color);
   pgWriteCmd(c->device,PGCANVAS_COLORCONV,1,4);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_bitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			  pghandle bitmap, pgu src_x, pgu src_y, short lgop) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_BITMAP,x,y,w,h);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,4,bitmap,lgop,src_x,src_y);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_tilebitmap(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			      pghandle bitmap, pgu src_x, pgu src_y,
			      pgu src_w, pgu src_h) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_TILEBITMAP,x,y,w,h);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,5,bitmap,src_x,src_y,src_w,src_h);
   return _pgfxcanvas_postprocess(c);
}

pgprim _pgfxcanvas_gradient(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			    pgu angle, pgcolor c1, pgcolor c2,
			    short translucent) {
   pgWriteCmd(c->device,PGCANVAS_GROP,5,PG_GROP_GRADIENT,x,y,w,h);
   pgWriteCmd(c->device,PGCANVAS_SETGROP,4,angle,c1,c2,translucent);
   return _pgfxcanvas_postprocess(c);
}

void _pgfxcanvas_setcolor(pgcontext c, pgcolor color) {
   c->color = color;
}

void _pgfxcanvas_update(pgcontext c) {
   pgWriteCmd(c->device,PGCANVAS_REDRAW,0);
   pgSubUpdate(c->device);
   if (c->flags == PGFX_IMMEDIATE)
     c->sequence = 0;
}

/******************************* Registration */

pgcontext pgNewCanvasContext(pghandle canvas,short mode) {
   static struct pgfx_lib l;  /* Should only need one of these */
   pgcontext ctx;
   
   ctx = _pg_malloc(sizeof(struct pgfx_context));
   if (!ctx) return NULL;
   memset(ctx,0,sizeof(struct pgfx_context));

   l.pixel      = _pgfxcanvas_pixel;
   l.line       = _pgfxcanvas_line;
   l.rect       = _pgfxcanvas_rect;
   l.dim        = _pgfxcanvas_dim;
   l.frame      = _pgfxcanvas_frame;
   l.slab       = _pgfxcanvas_slab;
   l.bar        = _pgfxcanvas_bar;
   l.text       = _pgfxcanvas_text;
   l.textv      = _pgfxcanvas_textv;
   l.bitmap     = _pgfxcanvas_bitmap;
   l.tilebitmap = _pgfxcanvas_tilebitmap;
   l.gradient   = _pgfxcanvas_gradient;
   l.setcolor   = _pgfxcanvas_setcolor;
   l.update     = _pgfxcanvas_update;
   
   ctx->lib = &l;
   ctx->device = canvas ? canvas : _pgdefault_widget;
   ctx->flags = mode;
   
   return ctx;
}

/* The End */
