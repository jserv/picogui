/* $Id: Ship.cpp,v 1.4 2002/11/26 19:18:07 micahjd Exp $
 *
 * Ship.cpp - Simple scriptable attack vessel
 *
 * Copyright (C) 2002 Micah Dowty and David Trowbridge
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
 */

#include <GL/glu.h>
#include "Ship.h"
#include "PGTexture.h"

Ship::Ship(PythonInterpreter *py) : ScriptableObject(py) {
  shipTexture = new PGTexture("jetengine/ship");
  setAttr("pitch",20.0f);
  setAttr("roll",0.0f);
  setAttr("yaw",0.0f);
  setAttr("altitude",15.0f);
  setAttr("x",0.0f);
  setAttr("z",-30.0f);
  setAttr("scale",10.0f);
}

Ship::~Ship() {
  delete shipTexture;
}

void Ship::draw() {
  float scale = getAttrFloat("scale");
  glPushMatrix();

  glTranslatef(getAttrFloat("x"), 
	       getAttrFloat("altitude"),
	       getAttrFloat("z"));

  glRotatef( getAttrFloat("pitch"), 1,0,0);
  glRotatef(-getAttrFloat("yaw"),   0,1,0);
  glRotatef(-getAttrFloat("roll"),  0,0,1);

  glScalef(scale,scale,scale);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  shipTexture->bind();
  glColor3f(1,1,1);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0);
  glVertex3f(-1,0, -1);
  glTexCoord2f(1,0);
  glVertex3f(1,0, -1);
  glTexCoord2f(1,1);
  glVertex3f(1,0,1.3);
  glTexCoord2f(0,1);
  glVertex3f(-1,0,1.3);
  glEnd();
  glDisable(GL_TEXTURE_2D);
	    
  glPopMatrix();
}
