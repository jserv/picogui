/* $Id: ScriptableObject.h,v 1.5 2002/11/26 19:18:07 micahjd Exp $
 *
 * ScriptableObject.h - Interface for a base class for C++ objects
 *                      that have attributes accessable from Python
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

#ifndef _H_SCRIPTABLEOBJECT
#define _H_SCRIPTABLEOBJECT

#include "PythonInterpreter.h"


class ScriptableObject : public PyObject {
 public:
  /* This lets our class be both a C++ class and a python object */
  static PyTypeObject Type;

  ScriptableObject(PythonInterpreter *py);
  virtual ~ScriptableObject();

  /* Called from the Python thread when an attribute is set */
  virtual void onAttrSet(char *name, PyObject *value) {};

  /* C++ interface
   * Provides convenience functions for several types, uses C++ exeptions
   */
  void setAttr(char *name, PyObject *value);
  void setAttr(char *name, int value);
  void setAttr(char *name, char *value);
  void setAttr(char *name, float value);
  PyObject *getAttr(char *name);
  int getAttrInt(char *name);
  char *getAttrStr(char *name);
  float getAttrFloat(char *name);

 private:
  PythonInterpreter *py;

  /* Python dictionary shared between python and C++ interfaces */
  PyObject *dict;

  /* Python interface
   * Thread-safe wrapper with no C++ exceptions, calls onAttrSet
   */
  static PyObject *PyGetAttr(PyObject * PyObj, char *attr);
  static int PySetAttr(PyObject *PyObj, char *attr, PyObject *value);
  static void PyDestructor(PyObject *PyObj);
};

#endif /* _H_SCRIPTABLEOBJECT */
