/* $Id$
 *
 * gl_primitives.c - OpenGL driver for picogui
 *                   Implement standard picogui primitives using OpenGL
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

#include <pgserver/common.h>
#include <pgserver/gl.h>


void gl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
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

hwrcolor gl_getpixel(hwrbitmap dest,s16 x,s16 y) {
  u8 r,g,b;

  if (GL_LINEAR32(dest))    
    return linear32_getpixel(STDB(dest),x,y);

  /* Don't allow reading pixels from the OpenGL screen, it's much too slow.
   * We'll just flag this as red, so we'll notice if something's trying to use it.
   */
  return 0xFF0000;
}

void gl_rect(hwrbitmap dest,s16 x,s16 y,s16 w, s16 h, hwrcolor c,s16 lgop) {
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

void gl_slab(hwrbitmap dest,s16 x,s16 y,s16 w, hwrcolor c,s16 lgop) {
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

void gl_bar(hwrbitmap dest,s16 x,s16 y,s16 h, hwrcolor c,s16 lgop) {
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

void gl_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c,s16 lgop) {
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
void gl_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
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

void gl_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		s16 src_x, s16 src_y, s16 lgop) {

  if (GL_LINEAR32(dest) && GL_LINEAR32_SRC(src)) {
    linear32_blit(STDB(dest),x,y,w,h,STDB(src),src_x,src_y,lgop);
    return;
  }

  /* Blitting from an offscreen bitmap to the screen? */
  if (GL_LINEAR32_SRC(src)) {
    struct glbitmap *glsrc = (struct glbitmap *) src;
    float tx1,ty1,tx2,ty2;

    /* Calculate texture coordinates */
    tx1 = glsrc->tx1 + src_x * (glsrc->tx2 - glsrc->tx1) / glsrc->sb->w;
    ty1 = glsrc->ty1 + src_y * (glsrc->ty2 - glsrc->ty1) / glsrc->sb->h;
    tx2 = glsrc->tx1 + (src_x + w) * (glsrc->tx2 - glsrc->tx1) / glsrc->sb->w;
    ty2 = glsrc->ty1 + (src_y + h) * (glsrc->ty2 - glsrc->ty1) / glsrc->sb->h;
    
    /* Draw a texture-mapped quad
     */
    gl_lgop(lgop);
    glEnable(GL_TEXTURE_2D);
    gl_bind_texture(glsrc);
    glBegin(GL_QUADS);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
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

int gl_update_hook(void) {
  if (gl_global.allow_update)
    return 0;
  gl_global.need_update++;
  return 1;
}

void gl_sprite_show(struct sprite *spr) {
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

void gl_sprite_hide(struct sprite *spr) {
  /* No need to erase sprites */
}

void gl_sprite_update(struct sprite *spr) {
  gl_global.need_update++;
}

void gl_sprite_protectarea(struct pgquad *in,struct sprite *from) { 
  /* No need to protect for sprites */
}

int gl_grop_render_presetup_hook(struct divnode **div, struct gropnode ***listp,
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

/* No color conversion, don't premultiply alphas */
hwrcolor gl_color_pgtohwr(pgcolor c) {
  return c;
}
pgcolor gl_color_hwrtopg(pgcolor c) {
  return c;
}

int gl_grop_render_node_hook(struct divnode **div, struct gropnode ***listp,
				struct groprender *rend, struct gropnode *node) {
  hwrcolor c;
  struct fontdesc *fd;
  char *str;
 
  /* We still need to map the node as normal, this handles the canvas' automatic
   * scaling, for example.
   */
  gropnode_map(rend,node);

  /* Override normal picogui translation and clipping- the divnode translation
   * is handled with an opengl matrix.
   * We do need to handle the PG_GROPF_TRANSLATE flag here though
   */
  if (node->flags & PG_GROPF_TRANSLATE) {
    node->r.x += rend->translation.x;
    node->r.y += rend->translation.y;
  }

  /* Override a couple grops */
  switch (node->type) {

    /* Override frame since the default frame has extra clipping
     * code we don't need or want.
     */
  case PG_GROP_FRAME:
    if (node->flags & PG_GROPF_COLORED)
      c = node->param[0];
    else
      c = rend->color;
    vid->slab(rend->output,node->r.x,node->r.y,node->r.w,c,rend->lgop);
    vid->slab(rend->output,node->r.x,node->r.y+node->r.h-1,node->r.w,c,rend->lgop);
    vid->bar(rend->output,node->r.x,node->r.y,node->r.h,c,rend->lgop);
    vid->bar(rend->output,node->r.x+node->r.w-1,node->r.y,node->r.h,c,rend->lgop);
    break;

  default:
    return 1;    /* Use normal code */
  }

  /* Override normal handling */
  node->type = PG_LGOP_NONE;
  return 1;
}

int gl_grop_render_postsetup_hook(struct divnode **div, struct gropnode ***listp,
				     struct groprender *rend) {
  GLdouble eqn[4];
  
  /* Set up clipping planes */

  glPushMatrix();
  glTranslatef(rend->clip.x1,0,0);
  eqn[0] = 1;
  eqn[1] = 0;
  eqn[2] = 0;
  eqn[3] = 0;
  glClipPlane(GL_CLIP_PLANE0,eqn);
  glPopMatrix();
  
  glPushMatrix();
  glTranslatef(rend->clip.x2+1,0,0);
  eqn[0] = -1;
  eqn[1] = 0;
  eqn[2] = 0;
  eqn[3] = 0;
  glClipPlane(GL_CLIP_PLANE1,eqn);
  glEnable(GL_CLIP_PLANE1);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0,rend->clip.y1,0);
  eqn[0] = 0;
  eqn[1] = 1;
  eqn[2] = 0;
  eqn[3] = 0;
  glClipPlane(GL_CLIP_PLANE2,eqn);
  glEnable(GL_CLIP_PLANE2);
  glPopMatrix();
  
  glPushMatrix();
  glTranslatef(0,rend->clip.y2+1,0);
  eqn[0] = 0;
  eqn[1] = -1;
  eqn[2] = 0;
  eqn[3] = 0;
  glClipPlane(GL_CLIP_PLANE3,eqn);
  glEnable(GL_CLIP_PLANE3);
  glPopMatrix();

  /* Push an OpenGL translation matrix to move to the divnode's origin */
  glPushMatrix();
  glTranslatef(rend->output_rect.x, rend->output_rect.y, 0);

  /* Move the clipping rectangle to be relative to this new matrix */
  rend->clip.x1 -= rend->output_rect.x;
  rend->clip.y1 -= rend->output_rect.y;
  rend->clip.x2 -= rend->output_rect.x;
  rend->clip.y2 -= rend->output_rect.y;

  return 0;
}

void gl_grop_render_end_hook(struct divnode **div, struct gropnode ***listp,
				struct groprender *rend) {
  /* Clean up */
  glPopMatrix();
}

void gl_multiblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		     hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 xo, s16 yo, s16 lgop) {
  s16 i,j;
  int blit_x, blit_y, blit_w, blit_h, blit_src_x, blit_src_y;
  int full_line_y = -1;
  struct glbitmap *glsrc = (struct glbitmap *) src;

  if (!(sw && sh)) return;

  /* Under just the right conditions, we can have OpenGL wrap the texture. We have to
   * be blitting from the entire source texture, and it has to be using the entire OpenGL
   * texture (therefore a power of 2 in each dimension)
   */
  if (sw==glsrc->sb->w && sh==glsrc->sb->h && sw==glsrc->tw && sh==glsrc->th) {
    /* Make sure this is actually a tiled blit, so we don't introduce artifacts when
     * doing normal blits from things that are a power of two
     */
    if (xo || yo || sw!=w || sh!=h) {
       glBindTexture(GL_TEXTURE_2D, glsrc->texture);
       glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
       glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    }
    gl_blit(dest,x,y,w,h,src,sx,sy,lgop);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    return;
  }
  
  /* Split the tiled blit up into individual blits clipped against the destination.
   * We do y clipping once per line, since only x coordinates change in the inner loop
   */
  
  for (j=-yo;j<h;j+=sh) {
    blit_y = y+j;
    blit_h = sh;
    blit_src_y = sy;
    if (j<0) {
      blit_y = y;
      blit_h += j;
      blit_src_y -= j;
    }
    if (blit_y + blit_h > y + h)
      blit_h = y + h - blit_y;
    
    for (i=-xo;i<w;i+=sw) {
      blit_x = x+i;
      blit_w = sw;
      blit_src_x = sx;
      if (i<0) {
	blit_x = x;
	blit_w += i;
	blit_src_x -= i;
      }
      if (blit_x + blit_w > x + w)
	blit_w = x + w - blit_x;
      
      (*vid->blit) (dest,blit_x,blit_y,blit_w,blit_h,src,blit_src_x,blit_src_y,lgop);
    }
    if (blit_h == sh)
      full_line_y = blit_y;
  }
}

void gl_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius) {
  int i,j;

  /* Convert the radius to a power of 2 */
  for (i=0,j=radius;j!=1;i++)
    j >>= 1;

  gl_feedback(x,y,w,h,PG_LGOP_NONE,GL_NEAREST_MIPMAP_NEAREST,GL_BACK,GL_TRUE,i);
}

/* The End */

