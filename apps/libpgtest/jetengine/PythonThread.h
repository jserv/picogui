/* The thread executing our python scripting */

#ifndef _H_PYTHONTHREAD
#define _H_PYTHONTHREAD

extern "C" {
#include <SDL/SDL_thread.h>
#include <Python.h>
}
#include "SimpleException.h"


class PythonThread {
 public:
  PythonThread(char *path, char *modulename);
  ~PythonThread();

  int threadHandler(void);

 private:
  void addPath(char *path);

  SDL_Thread *thread;
  PyObject *module, *args;
  bool running;
};

class PythonException : public SimpleException {
 public:
  virtual void show(void);
};


#endif /* _H_PYTHONTHREAD */
