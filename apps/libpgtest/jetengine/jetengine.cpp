/* Main program for a more complex example of pgserver embedding */

#include <stdio.h>
#include <GL/gl.h>
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

    pythread.addObject("world",&world);
    pythread.run();

    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50, 640.0/480.0, 1, 10000);
    glMatrixMode(GL_MODELVIEW);

    PGTexture ship("jetengine/ship");
    while (pgserver.mainloopIsRunning()) {

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glClear(GL_DEPTH_BUFFER_BIT);
      glShadeModel(GL_SMOOTH);

      glLoadIdentity();	
      glTranslatef(0,-20,-5);

      world.draw();

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

