/* The thread executing our python scripting */

#ifndef _H_PYTHONTHREAD
#define _H_PYTHONTHREAD

extern "C" {
#include <SDL/SDL_thread.h>
#include <Python.h>
}

class PythonThread {
 public:
  PythonThread(char *path="script", char *modulename="game");
  ~PythonThread();

  void addObject(char *name, PyObject *object);
  void run(char *function="thread");

  void threadHandler(void);

 private:
  void addPath(char *path);
  SDL_Thread *thread;
  PyObject *module;
  char *function;
};


#endif /* _H_PYTHONTHREAD */
