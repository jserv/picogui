/* Initialization for the python interpreter */

#include "PythonInterpreter.h"

PythonInterpreter::PythonInterpreter(int argc, char **argv) {
  Py_SetProgramName(argv[0]);
  Py_Initialize();
};

PythonInterpreter::~PythonInterpreter() {
  Py_Finalize();
}

void PythonException::show(void) {
  PyErr_Print();
}
