/* Camera position and tilt control */

#include <GL/glu.h>
#include "Camera.h"

Camera::Camera(PythonInterpreter *py) : ScriptableObject(py) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50, 640.0/480.0, 1, 10000);
  glMatrixMode(GL_MODELVIEW);

  setAttr("pitch",0.0f);
  setAttr("roll",0.0f);
  setAttr("yaw",0.0f);
  setAttr("altitude",20.0f);
  setAttr("x",0.0f);
  setAttr("z",-5.0f);
}

void Camera::setMatrix() {
  glLoadIdentity();
  glTranslatef( getAttrFloat("x"), 
	       -getAttrFloat("altitude"),
	        getAttrFloat("z"));

  glRotatef( getAttrFloat("pitch"), 1,0,0);
  glRotatef(-getAttrFloat("yaw"),   0,1,0);
  glRotatef(-getAttrFloat("roll"),  0,0,1);
}
