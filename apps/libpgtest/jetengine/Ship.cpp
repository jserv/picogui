/* Simple attack vessel */

#include <GL/glu.h>
#include "Ship.h"
#include "PGTexture.h"

Ship::Ship() {
  shipTexture = new PGTexture("jetengine/ship");
  setAttr("pitch",20.0f);
  setAttr("roll",0.0f);
  setAttr("yaw",0.0f);
  setAttr("altitude",15.0f);
  setAttr("x",0.0f);
  setAttr("z",-30.0f);
  setAttr("scale",10.0f);
}

Ship::~Ship() {
  delete shipTexture;
}

void Ship::draw() {
  float scale = getAttrFloat("scale");
  glPushMatrix();

  glTranslatef(getAttrFloat("x"), 
	       getAttrFloat("altitude"),
	       getAttrFloat("z"));

  glRotatef( getAttrFloat("pitch"), 1,0,0);
  glRotatef(-getAttrFloat("yaw"),   0,1,0);
  glRotatef(-getAttrFloat("roll"),  0,0,1);

  glScalef(scale,scale,scale);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  shipTexture->bind();
  glColor3f(1,1,1);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0);
  glVertex3f(-1,0, -1);
  glTexCoord2f(1,0);
  glVertex3f(1,0, -1);
  glTexCoord2f(1,1);
  glVertex3f(1,0,1.3);
  glTexCoord2f(0,1);
  glVertex3f(-1,0,1.3);
  glEnd();
  glDisable(GL_TEXTURE_2D);
	    
  glPopMatrix();
}
