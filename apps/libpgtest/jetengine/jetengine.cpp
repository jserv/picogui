/* Main program for a more complex example of pgserver embedding */

#include <stdio.h>
#include "EmbeddedPGserver.h"
#include "PythonThread.h"

int main(int argc, char **argv) {
  try {
    EmbeddedPGserver pgserver(argc, argv);
    PythonThread pythread("./script","game");

    while (pgserver.mainloopIsRunning()) {
      pgserver.mainloopIteration();
    }  
  }
  catch (SimpleException &e) {
    e.show();
    return 1;
  }
  return 0;
}
