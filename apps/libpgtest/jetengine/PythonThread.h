/* The thread executing our python scripting */

#ifndef _H_PYTHONTHREAD
#define _H_PYTHONTHREAD

extern "C" {
#include <SDL/SDL_thread.h>
#include <Python.h>
}

class PythonThread {
 public:
  PythonThread(char *module);
  ~PythonThread();

  int threadHandler(void);

 private:
  SDL_Thread *thread;
  PyObject *dict, *iteration;
  bool running;
};

#endif /* _H_PYTHONTHREAD */
