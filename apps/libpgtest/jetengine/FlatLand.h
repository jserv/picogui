/* A very simple environment model */

#ifndef _H_FLATLAND
#define _H_FLATLAND

#include "ScriptableObject.h"
#include "PGTexture.h"

class FlatLand : public ScriptableObject {
 public:
  FlatLand();
  virtual ~FlatLand();

  void draw(void);
  void animate(float seconds);

 private:
  PGTexture *ground;
  float x;
};

#endif /* _H_FLATLAND */
