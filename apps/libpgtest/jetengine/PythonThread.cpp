/* The thread executing our python scripting */

#include "PythonThread.h"
#include "PythonInterpreter.h"

/* FIXME: used for the mainloop hack below */
extern "C" {
#include <pgserver/common.h>
#include <pgserver/init.h>
}


/* Wrapper so C code can call our handler */
int PythonThreadCallback(void *data) {
  PythonThread *t = (PythonThread*) data;

  try {
    t->threadHandler();
  }
  catch (PythonException &e) {
    e.show();
  }

  /* Signal the main thread to terminate 
   * FIXME: This is an ugly method, we should have a callback or something
   *        so this is handled by the EmbeddedPGserver class.
   */
  pgserver_mainloop_stop();
  
  return 0;
}

PythonThread::PythonThread(char *path, char *modulename) {
  thread = NULL;
  
  addPath(path);

  module = PyImport_ImportModule(modulename);
  if (!module)
    throw PythonException();
}

PythonThread::~PythonThread() {
  if (thread)
    SDL_KillThread(thread);
  if (module)
    Py_DECREF(module);
}

void PythonThread::run(char *function_) {
  function = function_;
  thread = SDL_CreateThread(&PythonThreadCallback, this);
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

void PythonThread::threadHandler(void) {
  PyObject *func, *args, *ret;

  func = PyObject_GetAttrString(module, function);
  if (!func)
    throw PythonException();

  args = Py_BuildValue("()");
  ret = PyEval_CallObject(func,args);
  if (!ret)
    throw PythonException();

  Py_DECREF(args);
  Py_DECREF(ret);
  Py_DECREF(func);
}

void PythonThread::addObject(char *name, PyObject *object) {
  Py_INCREF(object);
  if (PyModule_AddObject(module, name, object)<0)
    throw PythonException();
}

