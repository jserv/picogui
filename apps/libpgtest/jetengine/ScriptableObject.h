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
  virtual void onAttrSet(char *name, PyObject *value);

  /* C++ interface */
  PyObject *getAttr(char *name);
  int setAttr(char *name, PyObject *value);
  
 private:
  /* Python dictionary shared between python and C++ threads */
  PyObject *dict;
  SDL_mutex *dict_mutex;

  /* Python interface */
  static PyObject *PyGetAttr(PyObject * PyObj, char *attr);
  static int PySetAttr(PyObject *PyObj, char *attr, PyObject *value);
  static void PyDestructor(PyObject *PyObj);
};

#endif /* _H_SCRIPTABLEOBJECT */
