/* $Id: pgfx.h,v 1.2 2001/04/14 00:02:22 micahjd Exp $
 *
 * picogui/pgfx.h - The PicoGUI abstract graphics interface
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

/************ Data structures */

/* Unit for x,y coordinates */
typedef int pgu;
/* RGB color */
typedef unsigned long pgcolor;
/* A number to identify the generated primitive, if it is stored */
typedef int pgprim;


/* This defines a rendering method and a device to output to
 * The application must treat this structure as read-only!
 */
typedef struct pgfx_context {
   struct pgfx_lib *lib;   /* Pointers to the rendering functions */
   pghandle device;        /* Output device */
   pgcolor color;          /* Current color */
   pgu cx,cy;              /* Current position for moveto/lineto */
   unsigned long flags;    /* Depends on the lib */
} *pgcontext;


/* Defines a standard set of functions implemented in pgfx_lib
 * These functions are equal to the available gropnodes and their parameters
 * as implemented in pgserver/gcore/grop.c (except color is omitted)
 * 
 * There are some things that would be messy to implement in this high-level
 * interface and so are only available with pgWriteCmd:
 * 
 *  - PG_GROP_TEXTGRID
 *  - Gropnode morphing / complex color conversions
 */
struct pgfx_lib {
   /* Primitives */
   pgprim (*pixel)     (pgcontext c, pgu x,  pgu y);
   pgprim (*line)      (pgcontext c, pgu x1, pgu y1, pgu x2, pgu y2);
   pgprim (*rect)      (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h);
   pgprim (*dim)       (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h);
   pgprim (*frame)     (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h);
   pgprim (*slab)      (pgcontext c, pgu x,  pgu y,  pgu w);
   pgprim (*bar)       (pgcontext c, pgu x,  pgu y,  pgu h);
   pgprim (*text)      (pgcontext c, pgu x,  pgu y,  pghandle string,
			pghandle font);
   pgprim (*textv)     (pgcontext c, pgu x,  pgu y,  pghandle string,
			pghandle font);
   pgprim (*bitmap)    (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pghandle bitmap, pgu src_x, pgu src_y, short lgop);
   pgprim (*tilebitmap)(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pghandle bitmap, pgu src_x, pgu src_y, pgu src_w,
			pgu src_h);
   pgprim (*gradient)  (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pgu angle, pgcolor c1, pgcolor c2, short translucent);

   /* Support functions */
   void (*setcolor)(pgcontext c, pgcolor color);
   /* FIXME: add functions to manipulate grops after they're created */
};

/************ Primitives */

/* These macros make it easy to call the primitives with a familiar syntax 
 * See the above definition of pgfx_lib for the parameters */
#define pgPixel(a,b,c)                    (*(a)->lib->pixel)(a,b,c)
#define pgLine(a,b,c,d,e)                 (*(a)->lib->line)(a,b,c,d,e)
#define pgRect(a,b,c,d,e)                 (*(a)->lib->rect)(a,b,c,d,e)
#define pgDim(a,b,c,d,e)                  (*(a)->lib->dim)(a,b,c,d,e)
#define pgFrame(a,b,c,d,e)                (*(a)->lib->frame)(a,b,c,d,e)
#define pgSlab(a,b,c,d)                   (*(a)->lib->slab)(a,b,c,d)
#define pgBar(a,b,c,d)                    (*(a)->lib->bar)(a,b,c,d)
#define pgText(a,b,c,d,e)                 (*(a)->lib->text)(a,b,c,d,e)
#define pgTextv(a,b,c,d,e)                (*(a)->lib->textv)(a,b,c,d,e)
#define pgBitmap(a,b,c,d,e,f,g,h,i)       (*(a)->lib->bitmap)(a,b,c,d,e,f,g,h,i)
#define pgTilebitmap(a,b,c,d,e,f,g,h,i,j) (*(a)->lib->tilebitmap)(a,b,c,d,e,f,g,h,i,j)
#define pgGradient(a,b,c,d,e,f,g,h,i)     (*(a)->lib->gradient)(a,b,c,d,e,f,g,h,i)
#define pgSetcolor(a,b)                   (*(a)->lib->setcolor)(a,b)

/* Meta-primitives */
void    pgMoveto(pgcontext c, pgu x, pgu y);
pgprim  pgLineto(pgcontext c, pgu x, pgu y);

/************ Constants */

/* Modes for pgNewCanvasContext */
#define PGFX_IMMEDIATE    1     /* Primitives are drawn now, not stored */
#define PGFX_PERSISTENT   2     /* Primitives drawn and stored in groplist */

/************ Context management functions */

pgcontext pgNewCanvasContext(pghandle canvas,short mode);
void pgDeleteContext(pgcontext c);

/* The End */
