/* A very simple environment model */

#include <GL/glu.h>
#include "FlatLand.h"
#include "PGTexture.h"

FlatLand::FlatLand() {
  groundTexture = new PGTexture("jetengine/grass");
  setAttr("velocity",0.0f);
  setAttr("hrepeat",100.0f);
  setAttr("vrepeat",100.0f);
  x = 0;
}

FlatLand::~FlatLand() {
  delete groundTexture;
}

void FlatLand::animate(float seconds) {
  /* Update the position using our velocity attribute */
  x += getAttrFloat("velocity") * seconds;
}

void FlatLand::draw() {
  float hrepeat, vrepeat;

  /* Simple gradient sky */
  glBegin(GL_QUADS);
  glColor3f(0.851, 0.886, 0.918);
  glVertex3f(-1000, 0, -1000);
  glVertex3f(1000, 0, -1000);
  glColor3f(0.047, 0.357, 0.569);
  glVertex3f(1000, 500,-1000);
  glVertex3f(-1000, 500,-1000);
  glEnd();
  
  hrepeat = getAttrFloat("hrepeat");
  vrepeat = getAttrFloat("vrepeat");

  /* Our ground plane, using 'x' as a bias for the v texture coordinates */
  glEnable(GL_TEXTURE_2D);
  groundTexture->bind();
  glBegin(GL_QUADS);
  glColor3f(0.8,0.8,0.8);
  glTexCoord2f(0,x);
  glVertex3f(-1000,0, 0);
  glTexCoord2f(hrepeat,x);
  glVertex3f(1000,0, 0);
  glColor3f(0.3,0.3,0.3);
  glTexCoord2f(hrepeat,x+vrepeat);
  glVertex3f(1000,0,-1000);
  glTexCoord2f(0,x+vrepeat);
  glVertex3f(-1000,0,-1000);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}
