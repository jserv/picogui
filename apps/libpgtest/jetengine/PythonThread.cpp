/* The thread executing our python scripting */

#include "PythonThread.h"

/* Wrapper so C code can call our handler */
int PythonThreadCallback(void *data) {
  PythonThread *t = (PythonThread*) data;
  return t->threadHandler();
}

PythonThread::PythonThread(char *module) {
  Py_Initialize();

  dict = PyImport_ImportModule(module);
  if (!dict)
    throw "Can't open python module";

  iteration = PyObject_GetAttrString(dict, "iteration");  
  if (!iteration)
    throw "Can't find 'iteration' in python module";

  running = true;
  thread = SDL_CreateThread(&PythonThreadCallback, this);
}

PythonThread::~PythonThread() {
  running = false;
  SDL_WaitThread(thread,NULL);
}

int PythonThread::threadHandler(void) {
  while (running) {
    PyEval_CallObject(iteration,NULL);
  }
  return 0;
}

