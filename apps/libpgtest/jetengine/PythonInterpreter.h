/* Initialization for the python interpreter */

#ifndef _H_PYTHONINTERPRETER
#define _H_PYTHONINTERPRETER

extern "C" {
#include <Python.h>
}
#include "SimpleException.h"

class PythonInterpreter {
 public:
  PythonInterpreter(int argc, char **argv);

  PyThreadState *mainThreadState;
};

class PythonException : public SimpleException {
 public:
  virtual void show(void);
};


#endif /* _H_PYTHONINTERPRETER */
