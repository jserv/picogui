/* $Id: pgfx.h,v 1.6 2001/05/10 03:05:36 micahjd Exp $
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
   int sequence;
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
   pgprim (*frame)     (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h);
   pgprim (*slab)      (pgcontext c, pgu x,  pgu y,  pgu w);
   pgprim (*bar)       (pgcontext c, pgu x,  pgu y,  pgu h);
   pgprim (*text)      (pgcontext c, pgu x,  pgu y,  pghandle string);
   pgprim (*bitmap)    (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pghandle bitmap);
   pgprim (*tilebitmap)(pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pghandle bitmap);
   pgprim (*gradient)  (pgcontext c, pgu x,  pgu y,  pgu w,  pgu h,
			pgu angle, pgcolor c1, pgcolor c2);

   /* Nonvisual primitives */
   pgprim (*setcolor)(pgcontext c, pgcolor color);
   pgprim (*setfont)(pgcontext c, pghandle font);
   pgprim (*setlgop)(pgcontext c, short lgop);
   pgprim (*setangle)(pgcontext c, pgu angle);              /* For text */
   pgprim (*setsrc)(pgcontext c, pgu x,pgu y,pgu w,pgu h);  /* For bitmaps */
   pgprim (*setmapping)(pgcontext c, pgu x,pgu y,pgu w,pgu h,short type);
   
   /* Call to mark the device for updating. Because PGFX is
    * output-method-independant it is not necessary to call pgSubUpdate */
   void (*update)(pgcontext c);
   
   /* FIXME: add functions to manipulate grops after they're created */
};

/************ Primitives */

/* These macros make it easy to call the primitives with a familiar syntax 
 * See the above definition of pgfx_lib for the parameters */
#define pgPixel(a,b,c)                    (*(a)->lib->pixel)(a,b,c)
#define pgLine(a,b,c,d,e)                 (*(a)->lib->line)(a,b,c,d,e)
#define pgRect(a,b,c,d,e)                 (*(a)->lib->rect)(a,b,c,d,e)
#define pgFrame(a,b,c,d,e)                (*(a)->lib->frame)(a,b,c,d,e)
#define pgSlab(a,b,c,d)                   (*(a)->lib->slab)(a,b,c,d)
#define pgBar(a,b,c,d)                    (*(a)->lib->bar)(a,b,c,d)
#define pgText(a,b,c,d)                   (*(a)->lib->text)(a,b,c,d)
#define pgBitmap(a,b,c,d,e,f)             (*(a)->lib->bitmap)(a,b,c,d,e,f)
#define pgTileBitmap(a,b,c,d,e,f)         (*(a)->lib->tilebitmap)(a,b,c,d,e,f)
#define pgGradient(a,b,c,d,e,f,g,h)       (*(a)->lib->gradient)(a,b,c,d,e,f,g,h)
#define pgSetColor(a,b)                   (*(a)->lib->setcolor)(a,b)
#define pgSetFont(a,b)                    (*(a)->lib->setfont)(a,b)
#define pgSetLgop(a,b)                    (*(a)->lib->setlgop)(a,b)
#define pgSetAngle(a,b)                   (*(a)->lib->setangle)(a,b)
#define pgSetSrc(a,b,c,d,e)               (*(a)->lib->setsrc)(a,b,c,d,e)
#define pgSetMapping(a,b,c,d,e,f)         (*(a)->lib->setmapping)(a,b,c,d,e,f)
#define pgContextUpdate(a)                (*(a)->lib->update)(a)

/* Meta-primitives */
void    pgMoveTo(pgcontext c, pgu x, pgu y);
pgprim  pgLineTo(pgcontext c, pgu x, pgu y);

/************ Constants */

/* Modes for pgNewCanvasContext */
#define PGFX_IMMEDIATE    1     /* Primitives are drawn now, not stored */
#define PGFX_PERSISTENT   2     /* Primitives drawn and stored in groplist */

/************ Context management functions */

pgcontext pgNewCanvasContext(pghandle canvas,short mode);
void pgDeleteContext(pgcontext c);

/* The End */
