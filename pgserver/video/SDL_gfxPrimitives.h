/* 

 SDL_gfxPrimitives.h

 ****** Editor's Note ******

 This file is from the SDL_gfxPrimitives library:
  http://freshmeat.net/projects/sdl_gfxprimitives/

 This library is licensed under the LGPL or the GPL.

 It has been modified some for use in pgserver:
 Unnecessary functions were removed, bugs fixed, and
 the header file hacked up.

    -- Micah

 ***************************
 
*/

#ifndef _SDL_gfxPrimitives_h
#define _SDL_gfxPrimitives_h

#include <math.h>
#ifndef M_PI
 #define M_PI	3.141592654
#endif

int fastPixelColorNolockNoclip (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color);
int aalineColor (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
int lineColor(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);


#endif /* _SDL_gfxPrimitives_h */
