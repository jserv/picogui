/* Main program for a more complex example of pgserver embedding */

#include <stdio.h>
#include "EmbeddedPGserver.h"
#include "PythonThread.h"

int main(int argc, char **argv) {
  try {
    EmbeddedPGserver pgserver(argc, argv);
    PythonThread pythread("game");

    while (pgserver.mainloopIsRunning()) {
      pgserver.mainloopIteration();
    }  
  }
  catch (PicoGUIException &e) {
    e.show();
    return 1;
  }
  catch (const char *str) {
    printf("Error: %s\n",str);
    return 1;
  }
  return 0;
}
