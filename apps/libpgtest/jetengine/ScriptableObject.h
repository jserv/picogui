/* A base class that adds python scriptability. */

#ifndef _H_SCRIPTABLEOBJECT
#define _H_SCRIPTABLEOBJECT

extern "C" {
#include <SDL/SDL_thread.h>
#include <Python.h>
}


class ScriptableObject : public PyObject {
 public:
  /* This lets our class be both a C++ class and a python object */
  static PyTypeObject Type;

  ScriptableObject();
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
  /* Python dictionary shared between python and C++ interfaces */
  PyObject *dict;
  SDL_mutex *dict_mutex;

  /* Python interface
   * Thread-safe wrapper with no C++ exceptions, calls onAttrSet
   */
  static PyObject *PyGetAttr(PyObject * PyObj, char *attr);
  static int PySetAttr(PyObject *PyObj, char *attr, PyObject *value);
  static void PyDestructor(PyObject *PyObj);
};

#endif /* _H_SCRIPTABLEOBJECT */
