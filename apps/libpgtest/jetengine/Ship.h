/* Simple attack vessel */

#ifndef _H_SHIP
#define _H_SHIP

#include "ScriptableObject.h"
#include "PGTexture.h"

class Ship : public ScriptableObject {
 public:
  Ship();
  virtual ~Ship();

  void draw(void);

 private:
  PGTexture *shipTexture;
};

#endif /* _H_SHIP */
