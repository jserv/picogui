/* $Id: PythonThread.cpp,v 1.10 2002/11/26 19:18:07 micahjd Exp $
 *
 * PythonThread.cpp - Implementation for the python thread that runs all our game code
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

PythonThread::PythonThread(PythonInterpreter *py_, char *path, char *modulename) {
  thread = NULL;
  py = py_;
  
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);

  addPath(path);

  module = PyImport_ImportModule(modulename);
  if (!module)
    throw PythonException();
  
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
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

/* This must be run while the python lock is held */
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

  PyEval_AcquireLock();
  threadState = PyThreadState_New(py->mainThreadState->interp);
  PyThreadState_Swap(threadState);

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

  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

void PythonThread::addObject(char *name, PyObject *object) {
  PyEval_AcquireLock();
  PyThreadState_Swap(py->mainThreadState);
  Py_INCREF(object);
  if (PyModule_AddObject(module, name, object)<0)
    throw PythonException();
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
}

