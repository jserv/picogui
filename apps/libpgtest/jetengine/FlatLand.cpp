/* $Id: FlatLand.cpp,v 1.9 2002/11/26 19:18:07 micahjd Exp $
 *
 * FlatLand.cpp - A very simple terrain model
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
#include "FlatLand.h"
#include "PGTexture.h"

FlatLand::FlatLand(PythonInterpreter *py) : ScriptableObject(py) {
  groundTexture = new PGTexture("jetengine/grass");
  setAttr("velocity",0.0f);
  setAttr("hrepeat",100.0f);
  setAttr("vrepeat",100.0f);
  x = 0;
}

FlatLand::~FlatLand() {
  delete groundTexture;
}

void FlatLand::animate(float seconds) {
  /* Update the position using our velocity attribute */
  x += getAttrFloat("velocity") * seconds;
}

void FlatLand::draw() {
  float hrepeat, vrepeat;

  hrepeat = getAttrFloat("hrepeat");
  vrepeat = getAttrFloat("vrepeat");

  /* Our ground plane, using 'x' as a bias for the v texture coordinates */
  glEnable(GL_TEXTURE_2D);
  groundTexture->bind();
  glBegin(GL_QUADS);
  glColor3f(0.8,0.8,0.8);
  glTexCoord2f(0,x);
  glVertex3f(-1000,0, 0);
  glTexCoord2f(hrepeat,x);
  glVertex3f(1000,0, 0);
  glColor3f(0.3,0.3,0.3);
  glTexCoord2f(hrepeat,x+vrepeat);
  glVertex3f(1000,0,-1000);
  glTexCoord2f(0,x+vrepeat);
  glVertex3f(-1000,0,-1000);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}
