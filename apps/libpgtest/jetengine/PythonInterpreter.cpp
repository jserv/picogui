/* Initialization for the python interpreter */

#include "PythonInterpreter.h"

PythonInterpreter::PythonInterpreter(int argc, char **argv) {
  Py_SetProgramName(argv[0]);
  Py_Initialize();
  PyEval_InitThreads();
  mainThreadState = PyThreadState_Get();
  PyEval_ReleaseLock();
};

void PythonException::show(void) {
  PyErr_Print();
}
