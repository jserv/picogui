/* Main program for a more complex example of pgserver embedding */

#include <stdio.h>
#include <GL/gl.h>
#include <SDL/SDL.h>
#include "EmbeddedPGserver.h"
#include "PythonThread.h"
#include "PythonInterpreter.h"
#include "PGTexture.h"
#include "FlatLand.h"
#include "ScriptableObject.h"


int main(int argc, char **argv) {
  PythonInterpreter py(argc, argv);
  try {
    EmbeddedPGserver pgserver(argc, argv);
    PythonThread pythread;
    FlatLand world;
    u32 old_ticks, new_ticks;
    float frame_time;

    pythread.addObject("world",&world);
    pythread.run();

    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50, 640.0/480.0, 1, 10000);
    glMatrixMode(GL_MODELVIEW);

    PGTexture ship("jetengine/ship");

    old_ticks = SDL_GetTicks();
    while (pgserver.mainloopIsRunning()) {
      new_ticks = SDL_GetTicks();
      frame_time = (new_ticks - old_ticks) / 1000.0f;
      old_ticks = new_ticks;

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glClear(GL_DEPTH_BUFFER_BIT);
      glShadeModel(GL_SMOOTH);

      glLoadIdentity();	
      glTranslatef(0,-20,-5);

      world.draw();
      world.animate(frame_time);

      /* Ship */
      glTranslatef(0,12,-30);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBlendEquation(GL_FUNC_ADD);
      ship.bind();
      glColor3f(1,1,1);
      glBegin(GL_QUADS);
      glTexCoord2f(0,0);
      glVertex3f(-10,5, -10);
      glTexCoord2f(1,0);
      glVertex3f(10,5, -10);
      glTexCoord2f(1,1);
      glVertex3f(10,0,10);
      glTexCoord2f(0,1);
      glVertex3f(-10,0,10);
      glEnd();
      glDisable(GL_TEXTURE_2D);

      pgserver.mainloopIteration();
    }  
  }
  catch (SimpleException &e) {
    e.show();
    return 1;
  }
  return 0;
}

