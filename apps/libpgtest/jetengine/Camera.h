/* Camera position and tilt control */

#ifndef _H_CAMERA
#define _H_CAMERA

#include "ScriptableObject.h"

class Camera : public ScriptableObject {
 public:
  Camera();
  void setMatrix(void);
};

#endif /* _H_CAMERA */
