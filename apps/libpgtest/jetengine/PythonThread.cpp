/* The thread executing our python scripting */

#include "PythonThread.h"

/* Wrapper so C code can call our handler */
int PythonThreadCallback(void *data) {
  PythonThread *t = (PythonThread*) data;
  return t->threadHandler();
}

PythonThread::PythonThread(char *path,char *modulename) {
  Py_Initialize();
  addPath(path);

  module = PyImport_ImportModule(modulename);
  if (!module)
    throw PythonException();
  
  args  = Py_BuildValue("()");
  running = true;
  thread = SDL_CreateThread(&PythonThreadCallback, this);
}

PythonThread::~PythonThread() {
  running = false;
  SDL_WaitThread(thread,NULL);
  Py_Finalize();
}

void PythonThread::addPath(char *path) {
  PyObject *sysmodule, *pathlist, *newpath;

  sysmodule = PyImport_ImportModule("sys");
  if (!sysmodule)
    throw PythonException();
  
  pathlist = PyObject_GetAttrString(sysmodule, "path");
  if (!pathlist)
    throw PythonException();
  
  newpath = PyString_FromString(path);
  if (!newpath)
    throw PythonException();

  if (PyList_Append(pathlist, newpath))
    throw PythonException();

  Py_DECREF(sysmodule);
  Py_DECREF(pathlist);
  Py_DECREF(newpath);
}

int PythonThread::threadHandler(void) {
  PyEval_CallObject(module,args);
  return 0;
}

void PythonException::show(void) {
  PyErr_Print();
}

