/* $Id$
 *
 * gl_grop.c - OpenGL driver for picogui
 *             This handles all new gropnodes that export OpenGL interfaces to
 *             clients and themes.   
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
#include <picogui/opengl.h>

/* Macro to convert fixed point back to floating point */
#define UNFIX(x) (((s32)(x))/65536.0f)
#define UNFIX16(x,y) UNFIX( (((u16)(x))<<16) | ((u16)(y)) )

void gl_grop_handler(struct groprender *r, struct gropnode *n) {
  struct glbitmap *glb;
  GLfloat vector[4];

  switch (n->type) {

  case PG_GROP_GL_BINDTEXTURE:
    if (!iserror(rdhandle((void**)&glb,PG_TYPE_BITMAP,-1,n->param[0])) && glb)
      gl_bind_texture(glb);
    break;

  case PG_GROP_GL_ENABLE:
    glEnable(n->param[0]);
    break;

  case PG_GROP_GL_DISABLE:
    glDisable(n->param[0]);
    break;

  case PG_GROP_GL_DEPTHFUNC:
    glDepthFunc(n->param[0]);
    break;

  case PG_GROP_GL_SHADEMODEL:
    glShadeModel(n->param[0]);
    break;

  case PG_GROP_GL_MATRIXMODE:
    glMatrixMode(n->param[0]);
    break;

  case PG_GROP_GL_LOADIDENTITY:
    glLoadIdentity();
    break;

  case PG_GROP_GL_PUSHMATRIX:
    glPushMatrix();
    break;

  case PG_GROP_GL_POPMATRIX:
    glPopMatrix();
    break;

  case PG_GROP_GL_TRANSLATEF:
    glTranslatef(UNFIX(n->param[0]),
		 UNFIX(n->param[1]),
		 UNFIX(n->param[2]));
    break;

  case PG_GROP_GL_ROTATEF:
    glRotatef(UNFIX16(n->r.x,n->r.y),
	      UNFIX(n->param[0]),
	      UNFIX(n->param[1]),
	      UNFIX(n->param[2]));
    break;

  case PG_GROP_GL_SCALEF:
    glScalef(UNFIX(n->param[0]),
	     UNFIX(n->param[1]),
	     UNFIX(n->param[2]));
    break;

  case PG_GROP_GL_BEGIN: 
    glBegin(n->param[0]);
    break;

  case PG_GROP_GL_TEXCOORD2F:
    glTexCoord2f(UNFIX(n->param[0]),
		 UNFIX(n->param[1]));
    break;

  case PG_GROP_GL_VERTEX3F:
    glVertex3f(UNFIX(n->param[0]),
	       UNFIX(n->param[1]),
	       UNFIX(n->param[2]));
    break;

  case PG_GROP_GL_END:
    glEnd();
    break;

  case PG_GROP_GL_HINT:
    glHint(n->param[0],n->param[1]);
    break;

  case PG_GROP_GL_NORMAL3F:
    glNormal3f(UNFIX(n->param[0]),
	       UNFIX(n->param[1]),
	       UNFIX(n->param[2]));    
    break;

  case PG_GROP_GL_LIGHTFV:
    vector[0] = UNFIX(n->param[0]);
    vector[1] = UNFIX(n->param[1]);
    vector[2] = UNFIX(n->param[2]);
    vector[3] = UNFIX16(n->r.w,n->r.h);
    glLightfv(n->r.x,n->r.y,(const GLfloat *)&vector);
    break;

  case PG_GROP_GL_MATERIALFV:
    vector[0] = UNFIX(n->param[0]);
    vector[1] = UNFIX(n->param[1]);
    vector[2] = UNFIX(n->param[2]);
    vector[3] = UNFIX16(n->r.w,n->r.h);
    glMaterialfv(n->r.x,n->r.y,(const GLfloat *)&vector);
    break;

  case PG_GROP_GL_TEXGENFV:
    vector[0] = UNFIX(n->param[0]);
    vector[1] = UNFIX(n->param[1]);
    vector[2] = UNFIX(n->param[2]);
    vector[3] = UNFIX16(n->r.w,n->r.h);
    glTexGenfv(n->r.x,n->r.y,(const GLfloat *)&vector);
    break;

  case PG_GROP_GL_TEXGENI:
    glTexGeni(n->param[0], n->param[1], n->param[2]);
    break;

  case PG_GROP_GL_MATERIALI:
    glMateriali(n->param[0], n->param[1], n->param[2]);
    break;

  case PG_GROP_GL_MATRIX_PIXELCOORD:
    gl_matrix_pixelcoord();
    break;

  case PG_GROP_GL_COLOR:
    gl_color(n->param[0]);
    break;

  case PG_GROP_GL_BLENDFUNC:
    glBlendFunc(n->param[0],n->param[1]);
    break;

  case PG_GROP_GL_FEEDBACK:
    gl_feedback(n->r.x, n->r.y, n->r.w, n->r.h,
		r->lgop, n->param[0], n->param[1],
		(n->param[2]>>16) & 0xFFFF, (n->param[2] & 0xFFFF));
    break;

  case PG_GROP_GL_TEXPARAMETERI:
    glTexParameteri(n->param[0],n->param[1],n->param[2]);
    break;

  }
}

/* The End */









