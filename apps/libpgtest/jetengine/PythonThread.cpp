/* The thread executing our python scripting */

#include "PythonThread.h"

/* FIXME: used for the mainloop hack below */
extern "C" {
#include <pgserver/common.h>
#include <pgserver/init.h>
}


/* Wrapper so C code can call our handler */
int PythonThreadCallback(void *data) {
  PythonThread *t = (PythonThread*) data;
  return t->threadHandler();
}

PythonThread::PythonThread(char *path,char *modulename_) {
  modulename = modulename_;
  Py_Initialize();
  addPath(path);
  
  thread = SDL_CreateThread(&PythonThreadCallback, this);
}

PythonThread::~PythonThread() {
  SDL_KillThread(thread);
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
  /* FIXME: Pass errors like this back to the other thread */
  if (!PyImport_ImportModule(modulename))
    PyErr_Print();

  /* Signal the main thread to terminate 
   * FIXME: This is an ugly method, we should have a callback or something
   *        so this is handled by the EmbeddedPGserver class.
   */
  pgserver_mainloop_stop();
  
  return 0;
}

void PythonException::show(void) {
  PyErr_Print();
}

