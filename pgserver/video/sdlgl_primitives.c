/* $Id: sdlgl_primitives.c,v 1.8 2002/03/03 20:05:29 micahjd Exp $
 *
 * sdlgl_primitives.c - OpenGL driver for picogui, using SDL for portability.
 *                      Implement standard picogui primitives using OpenGL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/sdlgl.h>


void sdlgl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_pixel(STDB(dest),x,y,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+1,y);
  glVertex2f(x+1,y+1);
  glVertex2f(x,y+1);
  glEnd();
}

/* This is _really_ damn slow, like the X11 getpixel,
 * but with all this fun hardware acceleration we shouldn't
 * actually have to use it much.
 */
hwrcolor sdlgl_getpixel(hwrbitmap dest,s16 x,s16 y) {
  u8 r,g,b;

  if (GL_LINEAR32(dest))
    return linear32_getpixel(STDB(dest),x,y);

  glReadPixels(x,y,1,1,GL_RED,GL_UNSIGNED_BYTE,&r);
  glReadPixels(x,y,1,1,GL_GREEN,GL_UNSIGNED_BYTE,&g);
  glReadPixels(x,y,1,1,GL_BLUE,GL_UNSIGNED_BYTE,&b);
  return mkcolor(r,g,b);
}

void sdlgl_rect(hwrbitmap dest,s16 x,s16 y,s16 w, s16 h, hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_rect(STDB(dest),x,y,w,h,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+w,y);
  glVertex2f(x+w,y+h);
  glVertex2f(x,y+h);
  glEnd();
}

void sdlgl_slab(hwrbitmap dest,s16 x,s16 y,s16 w, hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_slab(STDB(dest),x,y,w,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+w,y);
  glVertex2f(x+w,y+1);
  glVertex2f(x,y+1);
  glEnd();
}

void sdlgl_bar(hwrbitmap dest,s16 x,s16 y,s16 h, hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_bar(STDB(dest),x,y,h,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+1,y);
  glVertex2f(x+1,y+h);
  glVertex2f(x,y+h);
  glEnd();
}

void sdlgl_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_line(STDB(dest),x1,y1,x2,y2,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_LINES);
  gl_color(c);
  glVertex2f(x1,y1);
  glVertex2f(x2,y2);
  glEnd();
}

/* Yes indeedy, hardware accelerated gradients!
 *
 * Since we're free to use fancy floating point math, this gradient
 * function uses a different algorithm than def_gradient. A line of
 * equal color exists perpendicular to the gradient angle. This line L
 * passes through the center of the gradient's rectangle. All color
 * is a function of the distance from this line. We scale it so the colors
 * given will match the corners of the rectangle farthest from the
 * perpendicularl line.
 *
 * And then the fun part- let OpenGL do all the interpolation for us!
 */
void sdlgl_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		  pgcolor c1, pgcolor c2, s16 lgop) {
  float r_v1,g_v1,b_v1;
  float r_v2,g_v2,b_v2;
  float r_c,g_c,b_c;
  float r_s,g_s,b_s;
  float vx,vy;
  float cx,cy;
  float farthest, d1, d2, d3, d4;
  float theta = 0;

  if (GL_LINEAR32(dest)) {
    linear32_gradient(STDB(dest),x,y,w,h,angle,c1,c2,lgop);
    return;
  }

  gl_lgop(lgop);

  /* Use OpenGL's smooth shading for our interpolation */
  glShadeModel(GL_SMOOTH);

  /* Convert angle to radians */
  theta = -angle / 360.0f * 3.141592654 * 2.0f;

  /* Decode colors */
  r_v1 = getred(c1)/255.0f;
  g_v1 = getgreen(c1)/255.0f;
  b_v1 = getblue(c1)/255.0f;
  r_v2 = getred(c2)/255.0f;
  g_v2 = getgreen(c2)/255.0f;
  b_v2 = getblue(c2)/255.0f;
  
  /* Calculate center color */
  r_c = (r_v1 + r_v2)/2;
  g_c = (g_v1 + g_v2)/2;
  b_c = (b_v1 + b_v2)/2;

  /* Find a unit vector perpendicular to the specified angle */
  vx = sin(theta); 
  vy = cos(theta); 

  /* Find the center of the rectangle */
  cx = x + (w/2);
  cy = y + (h/2);

  /* Find the distance of the corner farthest from the center line */
  farthest =              d1 = gl_dist_line_to_point( x  ,y  , cx,cy,vx,vy );
  farthest = max(farthest,d2 = gl_dist_line_to_point( x+w,y  , cx,cy,vx,vy ));
  farthest = max(farthest,d3 = gl_dist_line_to_point( x+w,y+h, cx,cy,vx,vy ));
  farthest = max(farthest,d4 = gl_dist_line_to_point( x  ,y+h, cx,cy,vx,vy ));

  /* Find multipliers to convert distance from the line to distance
   * from the center color
   */
  r_s = (r_v2 - r_c) / farthest;
  g_s = (g_v2 - g_c) / farthest;
  b_s = (b_v2 - b_c) / farthest;
  
  /* Find the corresponding color for each vertex, let OpenGL interpolate.
   */
  glBegin(GL_QUADS);

  glColor3f(r_c + r_s * d1,
	    g_c + g_s * d1,
	    b_c + b_s * d1);
  glVertex2f(x,y);

  glColor3f(r_c + r_s * d2,
	    g_c + g_s * d2,
	    b_c + b_s * d2);
  glVertex2f(x+w,y);

  glColor3f(r_c + r_s * d3,
	    g_c + g_s * d3,
	    b_c + b_s * d3);
  glVertex2f(x+w,y+h);

  glColor3f(r_c + r_s * d4,
	    g_c + g_s * d4,
	    b_c + b_s * d4);
  glVertex2f(x,y+h);

  glEnd();  

  glShadeModel(GL_FLAT);
}

void sdlgl_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		s16 src_x, s16 src_y, s16 lgop) {

  if (GL_LINEAR32(dest) && GL_LINEAR32_SRC(src)) {
    linear32_blit(STDB(dest),x,y,w,h,STDB(src),src_x,src_y,lgop);
    return;
  }

  /* Blitting from an offscreen bitmap to the screen? */
  if (GL_LINEAR32_SRC(src)) {
    struct glbitmap *glsrc = (struct glbitmap *) src;
    float tx1,ty1,tx2,ty2;

    /* If we're tiling, create a cached tiled bitmap at the minimum
     * size if necessary, and use that to reduce the number of separate
     * quads we have to send to OpenGL
     */
    if ((w > glsrc->sb->w || h > glsrc->sb->h) && 
	(glsrc->sb->w < GL_TILESIZE_MIN || glsrc->sb->h < GL_TILESIZE_MIN)) {
      /* Create a pre-tiled image */
      if (!glsrc->tile) {
	int neww, newh;

	/* Calculate new width and height of the pretiled texture. */
	neww = glsrc->sb->w;
	if (neww < GL_TILESIZE_MIN)
	  neww = (GL_TILESIZE_IDEAL / glsrc->sb->w) * glsrc->sb->w;
	newh = glsrc->sb->h;
	if (newh < GL_TILESIZE_MIN)
	  newh = (GL_TILESIZE_IDEAL / glsrc->sb->h) * glsrc->sb->h;

	
	DBG("Expand tile from %dx%d to %dx%d\n",
	    glsrc->sb->w, glsrc->sb->h,neww,newh);	    

	vid->bitmap_new( (hwrbitmap*)&glsrc->tile,neww,newh, vid->bpp );
	def_blit((hwrbitmap)glsrc->tile,0,0,glsrc->tile->sb->w,
		 glsrc->tile->sb->h,src,0,0,PG_LGOP_NONE);
      }
      glsrc = glsrc->tile;
    }

    /* If we still have to tile, let defaulvbl do it
     */
    if (w > glsrc->sb->w || h > glsrc->sb->h) {
      def_blit(dest,x,y,w,h,(hwrbitmap) glsrc,src_x,src_y,lgop);
      return;
    }

    /* Make sure the bitmap has a corresponding texture */
    if (!glsrc->texture) {
      hwrbitmap tmpbit;
      u32 *p;
      u32 i;

      glGenTextures(1,&glsrc->texture);
      glBindTexture(GL_TEXTURE_2D, glsrc->texture);

      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,gl_global.texture_filtering);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,gl_global.texture_filtering);

      /* We have to round up to the nearest power of two...
       * FIXME: this is wasteful. We need a way to pack multiple
       *        glbitmaps into one texture.
       */
      glsrc->tw = gl_power2_round(glsrc->sb->w);
      glsrc->th = gl_power2_round(glsrc->sb->h);
      glsrc->tx1 = 0;
      glsrc->ty1 = 0;
      glsrc->tx2 = ((float)glsrc->sb->w) / ((float)glsrc->tw);
      glsrc->ty2 = ((float)glsrc->sb->h) / ((float)glsrc->th);
      
      /* Paste our image into it.
       * Note that the linear filtering depends on the value of the pixel adjacent to
       * the one being rendered as well, so to minimize rendering artifacts we copy
       * strips of the original bitmap to the sides of the new texture.
       */

      if (iserror(vid->bitmap_new(&tmpbit, glsrc->tw, glsrc->th, vid->bpp))) return;
      
      /* Texture itself */
      vid->blit(tmpbit, 0,0, glsrc->sb->w, glsrc->sb->h, src, 0,0, PG_LGOP_NONE);

      /* Left and right edges */
      if (glsrc->tw > glsrc->sb->w) {
	vid->blit(tmpbit, glsrc->sb->w,0, 1, glsrc->sb->h, src, glsrc->sb->w-1,0, PG_LGOP_NONE);
	vid->blit(tmpbit, glsrc->tw-1,0, 1,glsrc->sb->h, src, 0,0, PG_LGOP_NONE);
      }

      /* Top and bottom edges */
      if (glsrc->th > glsrc->sb->h) {
	vid->blit(tmpbit, 0,glsrc->sb->h, glsrc->sb->w, 1, src, 0,glsrc->sb->h-1, PG_LGOP_NONE);
	vid->blit(tmpbit, 0,glsrc->th-1, glsrc->sb->w,1, src, 0,0, PG_LGOP_NONE);
      }

      /* Corners */
      if (glsrc->tw > glsrc->sb->w && glsrc->th > glsrc->sb->h) {
	vid->blit(tmpbit, glsrc->sb->w,glsrc->sb->h, 1, 1, src, 
		  glsrc->sb->w-1,glsrc->sb->h-1, PG_LGOP_NONE);
	vid->blit(tmpbit, glsrc->tw-1,glsrc->th-1, 1,1, src, 0,0, PG_LGOP_NONE);
      }

      /* Now convert the alpha channel in our temporary bitmap. Any colors with the
       * PGCF_ALPHA flag need to have their alpha channels shifted over one bit,
       * other pixels get an alpha of 0xFF. Note that the original bitmap will still
       * have the PGCF_ALPHA-style colors, so detection of bitmaps with alpha channels
       * will work as usual.
       */
      i = glsrc->tw * glsrc->th;
      p = ((struct glbitmap*)tmpbit)->sb->bits;
      for (;i;i--,p++)
	if (*p & PGCF_ALPHA)
	  *p = (*p & 0x1FFFFFF) | ((*p & 0xFF000000)<<1);
	else
	  *p = *p | 0xFF000000;

      /* Send it to opengl, ditch our temporary */
      glTexImage2D(GL_TEXTURE_2D, 0, 4, glsrc->tw, glsrc->th, 0, 
		   GL_BGRA_EXT, GL_UNSIGNED_BYTE, ((struct glbitmap*)tmpbit)->sb->bits);
      vid->bitmap_free(tmpbit);

      //  gl_showtexture(glsrc->texture, glsrc->tw, glsrc->th);
    }      
    
    /* Calculate texture coordinates */
    tx1 = glsrc->tx1 + src_x * (glsrc->tx2 - glsrc->tx1) / glsrc->sb->w;
    ty1 = glsrc->ty1 + src_y * (glsrc->ty2 - glsrc->ty1) / glsrc->sb->h;
    tx2 = glsrc->tx1 + (src_x + w) * (glsrc->tx2 - glsrc->tx1) / glsrc->sb->w;
    ty2 = glsrc->ty1 + (src_y + h) * (glsrc->ty2 - glsrc->ty1) / glsrc->sb->h;
    
    /* Draw a texture-mapped quad
     */
    gl_lgop(lgop);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, glsrc->texture);
    glBegin(GL_QUADS);
    glColor3f(1.0f,1.0f,1.0f);
    glNormal3f(0.0f,0.0f,1.0f);
    glTexCoord2f(tx1,ty1);
    glVertex2f(x,y);
    glTexCoord2f(tx2,ty1);
    glVertex2f(x+w,y);
    glTexCoord2f(tx2,ty2);
    glVertex2f(x+w,y+h);
    glTexCoord2f(tx1,ty2);
    glVertex2f(x,y+h);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    return;
  }

  /* FIXME: can't blit from screen back to bitmap. Is this desired, or even possible? */
}

int sdlgl_update_hook(void) {
  if (gl_global.allow_update)
    return 0;
  gl_global.need_update++;
  return 1;
}

void sdlgl_sprite_show(struct sprite *spr) {
  if (!gl_global.allow_update)
    return;

  /* Display the sprite */
  if (spr->mask && *spr->mask) {
    VID(blit) (vid->display,spr->x,spr->y,spr->w,spr->h,
	       *spr->mask,0,0,PG_LGOP_AND);
    VID(blit) (vid->display,spr->x,spr->y,spr->w,spr->h,
	       *spr->bitmap,0,0,PG_LGOP_OR);
  }
  else
    VID(blit) (vid->display,spr->x,spr->y,spr->w,spr->h,
	       *spr->bitmap,0,0,spr->lgop);

}

void sdlgl_sprite_hide(struct sprite *spr) {
  /* No need to erase sprites */
}

void sdlgl_sprite_update(struct sprite *spr) {
  gl_global.need_update++;
}

void sdlgl_sprite_protectarea(struct quad *in,struct sprite *from) { 
  /* No need to protect for sprites */
}

int sdlgl_grop_render_presetup_hook(struct divnode **div, struct gropnode ***listp,
				    struct groprender *rend) {

  /* Don't bother with incremental or scroll-only gropnodes */
  (*div)->flags |= DIVNODE_NEED_REDRAW;

  /* If it's animated, rebuild it and note that we'll need another redraw soon */
  if ((*div)->flags & DIVNODE_ANIMATED) {
    div_rebuild(*div);
    gl_global.need_update++;
  }

  return 0;
}

/* The End */









