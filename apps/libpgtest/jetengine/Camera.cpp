/* $Id: Camera.cpp,v 1.4 2002/11/26 19:18:07 micahjd Exp $
 *
 * Camera.cpp - Scriptable class for camera control
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
#include "Camera.h"

Camera::Camera(PythonInterpreter *py) : ScriptableObject(py) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50, 640.0/480.0, 1, 10000);
  glMatrixMode(GL_MODELVIEW);

  setAttr("pitch",0.0f);
  setAttr("roll",0.0f);
  setAttr("yaw",0.0f);
  setAttr("altitude",20.0f);
  setAttr("x",0.0f);
  setAttr("z",-5.0f);
}

void Camera::setMatrix() {
  glLoadIdentity();
  glTranslatef( getAttrFloat("x"), 
	       -getAttrFloat("altitude"),
	        getAttrFloat("z"));

  glRotatef( getAttrFloat("pitch"), 1,0,0);
  glRotatef(-getAttrFloat("yaw"),   0,1,0);
  glRotatef(-getAttrFloat("roll"),  0,0,1);
}
