/* An OpenGL texture managed by pgserver */

#ifndef _H_PGTEXTURE
#define _H_PGTEXTURE

extern "C" {
#include <pgserver/common.h>
#include <pgserver/gl.h>
}


class PGTexture {
 public:
  PGTexture(const char *theme_object);  
  void bind(void);

 private:
  struct glbitmap *bitmap;
};

#endif /* _H_TEXTURE */
