/* A base class that adds python scriptability. */

#include "ScriptableObject.h"
#include "PythonInterpreter.h"

PyTypeObject ScriptableObject::Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"ScriptableObject",		/*tp_name*/
	sizeof(ScriptableObject),	/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	PyDestructor,	  		/*tp_dealloc*/
	0,			 	/*tp_print*/
	PyGetAttr, 			/*tp_getattr*/
	PySetAttr, 			/*tp_setattr*/
	0,			        /*tp_compare*/
	0,			        /*tp_repr*/
	0,			        /*tp_as_number*/
	0,		 	        /*tp_as_sequence*/
	0,			        /*tp_as_mapping*/
	0,			        /*tp_hash*/
	0,				/*tp_call */
};

ScriptableObject::ScriptableObject() {
  ob_type = &Type;
  _Py_NewReference(this);

  dict_mutex = SDL_CreateMutex();
  dict = PyDict_New();
}

ScriptableObject::~ScriptableObject() {
  Py_DECREF(dict);
  SDL_DestroyMutex(dict_mutex);
}

/* C++ wrappers have exception handling */
PyObject *ScriptableObject::getAttr(char *name) {
  PyObject *o;
  SDL_mutexP(dict_mutex);
  o = PyDict_GetItemString(dict,name);
  SDL_mutexV(dict_mutex);
  if (!o)
    throw PythonException();
  return o;
}

void ScriptableObject::setAttr(char *name, PyObject *value) {
  int r;
  SDL_mutexP(dict_mutex);
  r = PyDict_SetItemString(dict,name,value);
  SDL_mutexV(dict_mutex);
  if (r<0)
    throw PythonException();
}

/* Several convenience functions for set/getattr */
void ScriptableObject::setAttr(char *name, int value) {
  PyObject *o = Py_BuildValue("i",value);
  setAttr(name,o);
  Py_DECREF(o);
}

void ScriptableObject::setAttr(char *name, char *value) {
  PyObject *o = Py_BuildValue("s",value);
  setAttr(name,o);
  Py_DECREF(o);
}

void ScriptableObject::setAttr(char *name, float value) {
  PyObject *o = Py_BuildValue("f",value);
  setAttr(name,o);
  Py_DECREF(o);
}

int ScriptableObject::getAttrInt(char *name) {
  return PyInt_AsLong(getAttr(name));
}

char *ScriptableObject::getAttrStr(char *name) {
  return PyString_AsString(getAttr(name));
}

float ScriptableObject::getAttrFloat(char *name) {
  return PyFloat_AsDouble(getAttr(name));
}

/* Python wrappers have no exception handling, and call our virtual methods */
PyObject *ScriptableObject::PyGetAttr(PyObject * PyObj, char *attr) {
  ScriptableObject *my = (ScriptableObject*) PyObj;
  PyObject *o;
  SDL_mutexP(my->dict_mutex);
  o = PyDict_GetItemString(my->dict,attr);
  SDL_mutexV(my->dict_mutex);
  return o;
}

int ScriptableObject::PySetAttr(PyObject *PyObj, char *attr, PyObject *value) {
  ScriptableObject *my = (ScriptableObject*) PyObj;
  int r;
  SDL_mutexP(my->dict_mutex);
  r = PyDict_SetItemString(my->dict,attr,value);
  SDL_mutexV(my->dict_mutex);
  if (r>=0)
    my->onAttrSet(attr,value);
  return r;
}

void ScriptableObject::PyDestructor(PyObject *PyObj) {
  delete (ScriptableObject*) PyObj;
}
