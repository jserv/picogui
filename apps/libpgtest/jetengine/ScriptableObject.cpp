/* A base class that adds python scriptability. */

#include "ScriptableObject.h"


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

ScriptableObject::ScriptableObject(PythonInterpreter *py_) {
  py = py_;
  ob_type = &Type;
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  _Py_NewReference(this);
  dict = PyDict_New();
  if (!dict)
    throw PythonException();
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

ScriptableObject::~ScriptableObject() {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  Py_DECREF(dict);
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

/* C++ wrappers have exception handling */
PyObject *ScriptableObject::getAttr(char *name) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  PyObject *o = PyDict_GetItemString(dict,name);
  if (!o)
    throw PythonException();
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
  return o;
}

void ScriptableObject::setAttr(char *name, PyObject *value) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  if (PyDict_SetItemString(dict,name,value)<0)
    throw PythonException();
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

/* Several convenience functions for set/getattr */
void ScriptableObject::setAttr(char *name, int value) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  PyObject *o = Py_BuildValue("i",value);
  if (PyDict_SetItemString(dict,name,o)<0)
    throw PythonException();
  Py_DECREF(o);
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

void ScriptableObject::setAttr(char *name, char *value) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  PyObject *o = Py_BuildValue("s",value);
  if (PyDict_SetItemString(dict,name,o)<0)
    throw PythonException();
  Py_DECREF(o);
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

void ScriptableObject::setAttr(char *name, float value) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  PyObject *o = Py_BuildValue("f",value);
  if (PyDict_SetItemString(dict,name,o)<0)
    throw PythonException();
  Py_DECREF(o);
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

int ScriptableObject::getAttrInt(char *name) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  PyObject *o = PyDict_GetItemString(dict,name);
  if (!o)
    throw PythonException();  
  int i = PyInt_AsLong(o);
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
  return i;
}

char *ScriptableObject::getAttrStr(char *name) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  PyObject *o = PyDict_GetItemString(dict,name);
  if (!o)
    throw PythonException();
  char *s = PyString_AsString(o);
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
  return s;
}

float ScriptableObject::getAttrFloat(char *name) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  PyObject *o = PyDict_GetItemString(dict,name);
  if (!o)
    throw PythonException();
  float f = PyFloat_AsDouble(o);
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
  return f;
}

/* Python wrappers have no exception handling, and call our virtual methods */
PyObject *ScriptableObject::PyGetAttr(PyObject * PyObj, char *attr) {
  ScriptableObject *my = (ScriptableObject*) PyObj;
  PyObject *o;

  o = PyDict_GetItemString(my->dict,attr);
  Py_INCREF(o);

  return o;
}

int ScriptableObject::PySetAttr(PyObject *PyObj, char *attr, PyObject *value) {
  ScriptableObject *my = (ScriptableObject*) PyObj;
  int r;

  r = PyDict_SetItemString(my->dict,attr,value);

  if (r>=0)
    my->onAttrSet(attr,value);
  return r;
}

void ScriptableObject::PyDestructor(PyObject *PyObj) {
  delete (ScriptableObject*) PyObj;
}
