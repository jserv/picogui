/* The thread executing our python scripting */

#ifndef _H_PYTHONTHREAD
#define _H_PYTHONTHREAD

extern "C" {
#include <SDL/SDL_thread.h>
#include <Python.h>
}

class PythonThread {
 public:
  PythonThread(void);
  ~PythonThread();

  void addPath(char *path);
  void addObject(char *name, PyObject *object);
  void run(char *modulename);

  int threadHandler(void);

 private:
  SDL_Thread *thread;
  PyObject *globals;
  char *modulename;
};


#endif /* _H_PYTHONTHREAD */
