/* A base class that adds python scriptability. */

#include "ScriptableObject.h"
#include "PythonThread.h"

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

/* Called from the Python thread when an attribute is set */
void ScriptableObject::onAttrSet(char *name, PyObject *value) {
}

PyObject *ScriptableObject::getAttr(char *name) {
  PyObject *o;
  SDL_mutexP(dict_mutex);
  o = PyDict_GetItemString(dict,name);
  SDL_mutexV(dict_mutex);
  return o;
}

int ScriptableObject::setAttr(char *name, PyObject *value) {
  int r;
  SDL_mutexP(dict_mutex);
  r = PyDict_SetItemString(dict,name,value);
  SDL_mutexV(dict_mutex);
  if (r>=0)
    onAttrSet(name,value);
  return r;
}

PyObject *ScriptableObject::PyGetAttr(PyObject * PyObj, char *attr) {
  return ((ScriptableObject*) PyObj)->getAttr(attr);
}

int ScriptableObject::PySetAttr(PyObject *PyObj, char *attr, PyObject *value) {
  return ((ScriptableObject*) PyObj)->setAttr(attr,value);
}

void ScriptableObject::PyDestructor(PyObject *PyObj) {
  delete (ScriptableObject*) PyObj;
}
