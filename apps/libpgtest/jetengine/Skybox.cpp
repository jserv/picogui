#include <GL/glu.h>
#include "Skybox.h"

Skybox::Skybox(PythonInterpreter *py) : ScriptableObject(py) {
  sides[0] = new PGTexture("skyboxes/malrav11/left");
  sides[1] = new PGTexture("skyboxes/malrav11/right");
  sides[2] = new PGTexture("skyboxes/malrav11/front");
  sides[3] = new PGTexture("skyboxes/malrav11/back");
  sides[4] = new PGTexture("skyboxes/malrav11/top");
  sides[5] = new PGTexture("skyboxes/malrav11/bottom");

  for(int i = 0; i < 6; i++)
    lists[i] = glGenLists(1);

  float size = 750.0f;
  glNewList(lists[0], GL_COMPILE); {
    glBegin(GL_QUADS); {
      glTexCoord2f(1, 1); glVertex3f(size - 2, -size,  size);
      glTexCoord2f(1, 0); glVertex3f(size - 2,  size,  size);
      glTexCoord2f(0, 0); glVertex3f(size - 2,  size, -size);
      glTexCoord2f(0, 1); glVertex3f(size - 2, -size, -size);
    } glEnd();
  } glEndList();
  glNewList(lists[1], GL_COMPILE); {
    glBegin(GL_QUADS); {
      glTexCoord2f(1, 1); glVertex3f(-size + 2, -size, -size);
      glTexCoord2f(1, 0); glVertex3f(-size + 2,  size, -size);
      glTexCoord2f(0, 0); glVertex3f(-size + 2,  size,  size);
      glTexCoord2f(0, 1); glVertex3f(-size + 2, -size,  size);
    } glEnd();
  } glEndList();
  glNewList(lists[2], GL_COMPILE); {
    glBegin(GL_QUADS); {
      glTexCoord2f(0, 1); glVertex3f(-size, -size, -size + 2);
      glTexCoord2f(1, 1); glVertex3f( size, -size, -size + 2);
      glTexCoord2f(1, 0); glVertex3f( size,  size, -size + 2);
      glTexCoord2f(0, 0); glVertex3f(-size,  size, -size + 2);
    } glEnd();
  } glEndList();
  glNewList(lists[3], GL_COMPILE); {
    glBegin(GL_QUADS); {
      glTexCoord2f(1, 0); glVertex3f(-size,  size,  size - 2);
      glTexCoord2f(0, 0); glVertex3f( size,  size,  size - 2);
      glTexCoord2f(0, 1); glVertex3f( size, -size,  size - 2);
      glTexCoord2f(1, 1); glVertex3f(-size, -size,  size - 2);
    } glEnd();
  } glEndList();
  glNewList(lists[4], GL_COMPILE); {
    glBegin(GL_QUADS); {
      glTexCoord2f(1, 1); glVertex3f(-size, size - 2, -size);
      glTexCoord2f(1, 0); glVertex3f( size, size - 2, -size);
      glTexCoord2f(0, 0); glVertex3f( size, size - 2,  size);
      glTexCoord2f(0, 1); glVertex3f(-size, size - 2,  size);
    } glEnd();
  } glEndList();
  glNewList(lists[5], GL_COMPILE); {
    glBegin(GL_QUADS); {
      glTexCoord2f(0, 0); glVertex3f(-size, -size + 2,  size);
      glTexCoord2f(0, 1); glVertex3f( size, -size + 2,  size);
      glTexCoord2f(1, 1); glVertex3f( size, -size + 2, -size);
      glTexCoord2f(1, 0); glVertex3f(-size, -size + 2, -size);
    } glEnd();
  } glEndList();
}

Skybox::~Skybox() {
  delete sides[0];
  delete sides[1];
  delete sides[2];
  delete sides[3];
  delete sides[4];
  delete sides[5];
  glDeleteLists(lists[0], 1);
  glDeleteLists(lists[1], 1);
  glDeleteLists(lists[2], 1);
  glDeleteLists(lists[3], 1);
  glDeleteLists(lists[4], 1);
  glDeleteLists(lists[5], 1);
}

void Skybox::draw() {
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

  glColor3f(1, 1, 1);

  sides[0]->bind(); glCallList(lists[0]);
  sides[1]->bind(); glCallList(lists[1]);
  sides[2]->bind(); glCallList(lists[2]);
  sides[3]->bind(); glCallList(lists[3]);
  sides[4]->bind(); glCallList(lists[4]);
  sides[5]->bind(); glCallList(lists[5]);

  glDisable(GL_TEXTURE_2D);
}
