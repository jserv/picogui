/* Main program for a more complex example of pgserver embedding */

#include <stdio.h>
#include <GL/gl.h>
#include "EmbeddedPGserver.h"
#include "PythonThread.h"
#include "PythonInterpreter.h"
#include "PGTexture.h"
#include "ScriptableObject.h"


int main(int argc, char **argv) {
  PythonInterpreter py(argc, argv);
  try {
    EmbeddedPGserver pgserver(argc, argv);
    PythonThread pythread;
    
    pythread.addObject("foo",new ScriptableObject);
    pythread.run();

    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50, 640.0/480.0, 1, 10000);
    glMatrixMode(GL_MODELVIEW);

    int frame=0;
    PGTexture grass("jetengine/grass");
    PGTexture ship("jetengine/ship");
    while (pgserver.mainloopIsRunning()) {
      frame++;

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glClear(GL_DEPTH_BUFFER_BIT);
      glShadeModel(GL_SMOOTH);

      glLoadIdentity();	
      glTranslatef(0,-20,-5);

      /* Sky */
      glBegin(GL_QUADS);
      glColor3f(0.851, 0.886, 0.918);
      glVertex3f(-1000, 0, -1000);
      glVertex3f(1000, 0, -1000);
      glColor3f(0.047, 0.357, 0.569);
      glVertex3f(1000, 500,-1000);
      glVertex3f(-1000, 500,-1000);
      glEnd();

      /* Ground */
      float x = frame*0.01;
      glEnable(GL_TEXTURE_2D);
      grass.bind();
      glBegin(GL_QUADS);
      glColor3f(0.8,0.8,0.8);
      glTexCoord2f(0,x);
      glVertex3f(-1000,0, 0);
      glTexCoord2f(100,x);
      glVertex3f(1000,0, 0);
      glColor3f(0.3,0.3,0.3);
      glTexCoord2f(100,x+10);
      glVertex3f(1000,0,-1000);
      glTexCoord2f(0,x+10);
      glVertex3f(-1000,0,-1000);
      glEnd();
      glDisable(GL_TEXTURE_2D);

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

