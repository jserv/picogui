/* $Id: sdlgl_grop.c,v 1.3 2002/03/05 23:48:28 micahjd Exp $
 *
 * sdlgl_grop.c - OpenGL driver for picogui, using SDL for portability.
 *                This handles all new gropnodes that export OpenGL interfaces to
 *                clients and themes.   
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
#include <picogui/opengl.h>

/* Macro to convert fixed point back to floating point */
#define UNFIX(x) (((s32)(x))/65536.0f)
#define UNFIX16(x,y) UNFIX( (((u16)(x))<<16) | ((u16)(y)) )

void sdlgl_grop_handler(struct groprender *r, struct gropnode *n) {
  struct glbitmap *glb;
  GLfloat vector[4];

  switch (n->type) {

  case PG_GROP_GL_BINDTEXTURE:
    if (!iserror(rdhandle((void**)&glb,PG_TYPE_BITMAP,-1,n->param[0])) && glb) {
      gl_make_texture(glb);
      glBindTexture(GL_TEXTURE_2D,glb->texture);
    }
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
    glLightfv(n->r.x,n->r.y,&vector);
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

  }
}

/* The End */









