/* $Id: PythonThread.h,v 1.8 2002/11/26 19:18:07 micahjd Exp $
 *
 * PythonThread.h - Interface for the python thread that runs all our game code
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

#ifndef _H_PYTHONTHREAD
#define _H_PYTHONTHREAD

extern "C" {
#include <SDL/SDL_thread.h>
#include <Python.h>
}
#include "PythonInterpreter.h"

class PythonThread {
 public:
  PythonThread(PythonInterpreter *py, char *path="script", char *modulename="game");
  ~PythonThread();

  void addObject(char *name, PyObject *object);
  void run(char *function="thread");

  void threadHandler(void);

 private:
  void addPath(char *path);
  SDL_Thread *thread;
  PyObject *module;
  char *function;
  PythonInterpreter *py;
  PyThreadState *threadState;
};


#endif /* _H_PYTHONTHREAD */
