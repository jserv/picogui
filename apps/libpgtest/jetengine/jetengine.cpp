/* Main program for a more complex example of pgserver embedding */

#include <stdio.h>
#include <GL/gl.h>
#include <SDL/SDL.h>
#include "EmbeddedPGserver.h"
#include "PythonThread.h"
#include "PythonInterpreter.h"
#include "FlatLand.h"
#include "Ship.h"
#include "Camera.h"
#include "Skybox.h"


int main(int argc, char **argv) {
  PythonInterpreter py(argc, argv);
  try {
    EmbeddedPGserver pgserver(argc, argv);
    PythonThread pythread;
    FlatLand world;
    Camera camera;
    Ship ship;
    Skybox skybox;
    u32 old_ticks, new_ticks;
    float frame_time;

    pythread.addObject("world",&world);
    pythread.addObject("ship",&ship);
    pythread.addObject("camera",&camera);
    pythread.addObject("skybox",&skybox);
    pythread.run();

    old_ticks = SDL_GetTicks();
    while (pgserver.mainloopIsRunning()) {
      new_ticks = SDL_GetTicks();
      frame_time = (new_ticks - old_ticks) / 1000.0f;
      old_ticks = new_ticks;

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glClear(GL_DEPTH_BUFFER_BIT);
      glShadeModel(GL_SMOOTH);

      camera.setMatrix();
      skybox.draw();
      world.draw();
      ship.draw();
      world.animate(frame_time);

      pgserver.mainloopIteration();
    }  
  }
  catch (SimpleException &e) {
    e.show();
    return 1;
  }
  return 0;
}

