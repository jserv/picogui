#include <GL/glu.h>
#include "Skybox.h"

Skybox::Skybox(PythonInterpreter *py) : ScriptableObject(py) {
  left = new PGTexture("skyboxes/malrav11/left");
  right = new PGTexture("skyboxes/malrav11/right");
  front = new PGTexture("skyboxes/malrav11/front");
  back = new PGTexture("skyboxes/malrav11/back");
  top = new PGTexture("skyboxes/malrav11/top");
  bottom = new PGTexture("skyboxes/malrav11/bottom");
}

Skybox::~Skybox() {
  delete left;
  delete right;
  delete front;
  delete back;
  delete top;
  delete bottom;
}

void Skybox::draw() {
  float size = 750.0f;

  glPushMatrix();

  glEnable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

  glColor3f(1, 1, 1);

  right->bind();
  glBegin(GL_QUADS); {
    glTexCoord2f(1, 1); glVertex3f(-size + 2, -size, -size);
    glTexCoord2f(1, 0); glVertex3f(-size + 2,  size, -size);
    glTexCoord2f(0, 0); glVertex3f(-size + 2,  size,  size);
    glTexCoord2f(0, 1); glVertex3f(-size + 2, -size,  size);
  } glEnd();

  left->bind();
  glBegin(GL_QUADS); {
    glTexCoord2f(1, 1); glVertex3f(size - 2, -size,  size);
    glTexCoord2f(1, 0); glVertex3f(size - 2,  size,  size);
    glTexCoord2f(0, 0); glVertex3f(size - 2,  size, -size);
    glTexCoord2f(0, 1); glVertex3f(size - 2, -size, -size);
  } glEnd();

  top->bind();
  glBegin(GL_QUADS); {
    glTexCoord2f(1, 1); glVertex3f(-size, size - 2, -size);
    glTexCoord2f(1, 0); glVertex3f( size, size - 2, -size);
    glTexCoord2f(0, 0); glVertex3f( size, size - 2,  size);
    glTexCoord2f(0, 1); glVertex3f(-size, size - 2,  size);
  } glEnd();

  bottom->bind();
  glBegin(GL_QUADS); {
    glTexCoord2f(0, 0); glVertex3f(-size, -size + 2,  size);
    glTexCoord2f(0, 1); glVertex3f( size, -size + 2,  size);
    glTexCoord2f(1, 1); glVertex3f( size, -size + 2, -size);
    glTexCoord2f(1, 0); glVertex3f(-size, -size + 2, -size);
  } glEnd();

  back->bind();
  glBegin(GL_QUADS); {
    glTexCoord2f(1, 0); glVertex3f(-size,  size,  size - 2);
    glTexCoord2f(0, 0); glVertex3f( size,  size,  size - 2);
    glTexCoord2f(0, 1); glVertex3f( size, -size,  size - 2);
    glTexCoord2f(1, 1); glVertex3f(-size, -size,  size - 2);
  } glEnd();

  front->bind();
  glBegin(GL_QUADS); {
    glTexCoord2f(0, 1); glVertex3f(-size, -size, -size + 2);
    glTexCoord2f(1, 1); glVertex3f( size, -size, -size + 2);
    glTexCoord2f(1, 0); glVertex3f( size,  size, -size + 2);
    glTexCoord2f(0, 0); glVertex3f(-size,  size, -size + 2);
  } glEnd();

  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}
