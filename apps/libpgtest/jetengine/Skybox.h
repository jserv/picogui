#ifndef _H_SKYBOX
#define _H_SKYBOX

#include "ScriptableObject.h"
#include "PGTexture.h"

class Skybox : public ScriptableObject {
  public:
    Skybox(PythonInterpreter *py);
    virtual ~Skybox();

    void draw(void);

  private:
    PGTexture *left, *right, *front, *back, *top, *bottom;
};

#endif /* _H_SKYBOX */
