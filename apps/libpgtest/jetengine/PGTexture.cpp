/* An OpenGL texture managed by pgserver */

#include "PGTexture.h"
#include "EmbeddedPGserver.h"
#include <stdio.h>
extern "C" {
#include <pgserver/common.h>
#include <pgserver/handle.h>
#include <pgserver/svrtheme.h>
#include <pgserver/video.h>
#include <pgserver/pgstring.h>
#include <pgserver/gl.h>
}

class TextureException : public SimpleException {
public:
  TextureException(const char *object_) {
    object = object_;
  }
  virtual void show(void) {
    printf("Error loading texture from theme object '%s'\n",object);
  }
private:
  const char *object;
};

PGTexture::PGTexture(const char *theme_object) {
  s16 id;
  handle hbitmap;
  g_error e;

  /* Find the numeric ID associated with this theme object.
   * pgstring_tmpwrap is a wrapper that temporarily converts a given
   * C string into an ASCII pgstring object.
   */
  find_named_thobj(pgstring_tmpwrap(theme_object), &id);

  /* Find the handle of the loaded bitmap */
  hbitmap = theme_lookup(id, PGTH_P_BITMAP1);

  /* Dereference the handle. Note that if the theme is unloaded while
   * this class instance still exists, calling bind() will crash!
   * This could be fixed by moving this dereference step into bind(),
   * but at a small speed penalty.
   */
  e = rdhandle((void**) &bitmap, PG_TYPE_BITMAP, -1, hbitmap);
  if (iserror(e))
    throw PicoGUIException(e);

  if (!bitmap)
    throw TextureException(theme_object);

  /* PicoGUI clamps the texture by default, we'd rather wrap it */
  bind();
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

void PGTexture::bind(void) {
  gl_bind_texture(bitmap);
}

