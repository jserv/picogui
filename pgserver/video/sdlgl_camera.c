/* $Id: sdlgl_camera.c,v 1.1 2002/03/03 05:42:26 micahjd Exp $
 *
 * sdlgl_camera.c - OpenGL driver for picogui, using SDL for portability.
 *                  This file traps keyboard and mouse input for camera control
 *                  and other on-the-fly settings.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/sdlgl.h>

int sdlgl_key_event_hook(u32 *type, s16 *key, s16 *mods) {

  /* Entering/leaving modes */

  if ((*mods & PGMOD_CTRL) && (*mods & PGMOD_ALT) && *type==PG_TRIGGER_KEYDOWN)
    switch (*key) {

      /* Camera modes */

    case PGKEY_q:
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_TRANSLATE)
	gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
      else
	gl_global.camera_mode = SDLGL_CAMERAMODE_TRANSLATE;
      return 1;

    case PGKEY_e:
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_ROTATE)
	gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
      else
	gl_global.camera_mode = SDLGL_CAMERAMODE_ROTATE;
      return 1;

    case PGKEY_r:
      gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
      glLoadIdentity();
      gl_matrix_pixelcoord();
      return 1;

      /* Misc flags */

    case PGKEY_f:
      gl_global.showfps = !gl_global.showfps;
      return 1;

    case PGKEY_g:
      gl_global.grid = !gl_global.grid;
      return 1;

    default:
      return 0;
    }
     
  /* In a camera mode? */
 
  if (gl_global.camera_mode != SDLGL_CAMERAMODE_NONE) {
    /* Keep track of pressed keys */

    if (*type == PG_TRIGGER_KEYDOWN)
      gl_global.pressed_keys[*key] = 1;
    if (*type == PG_TRIGGER_KEYUP)
      gl_global.pressed_keys[*key] = 0;

    /* A couple keys should exit camera mode... */
    switch (*key) {
    case PGKEY_ESCAPE:
    case PGKEY_SPACE:
    case PGKEY_RETURN:
      gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
    }

    /* Trap events */
    return 1;
  }

  /* Pass the event */
  return 0;
}

int sdlgl_pointing_event_hook(u32 *type, s16 *x, s16 *y, s16 *btn) {
  float dx,dy,dz=0;
  s16 cursorx,cursory;
  float scale;

  /* Just pass the event if we're not in a camera mode */
  if (gl_global.camera_mode == SDLGL_CAMERAMODE_NONE)
    return 0;

  /* Get the physical position of PicoGUI's cursor */
  cursorx = cursor->x;
  cursory = cursor->y;
  VID(coord_physicalize)(&cursorx,&cursory);
  
  /* get the movement since last time and warp the mouse back */
  dx = *x - cursorx;
  dy = *y - cursory;
  SDL_WarpMouse(cursorx,cursory);

  /* Translate the mouse wheel into Z motion */
  if (*type == PG_TRIGGER_DOWN && (*btn & 8))
    dz = 20;
  if (*type == PG_TRIGGER_DOWN && (*btn & 16))
    dz = -20;

  scale = gl_get_key_scale();
  dx *= scale;
  dy *= scale;
  dz *= scale;

  switch (gl_global.camera_mode) {

  case SDLGL_CAMERAMODE_TRANSLATE:
    glTranslatef(dx,dy,dz);
    break;

  case SDLGL_CAMERAMODE_ROTATE:
    glRotatef(dy/10.0,1,0,0);
    glRotatef(dx/10.0,0,1,0);
    glRotatef(dz/10.0,0,0,1);
    break;

  }
  
  /* Absorb the event */
  return 1;
}

void gl_process_camera_keys(void) {
  float scale = gl_get_key_scale();

  /* Process camera movement keys */
  switch (gl_global.camera_mode) {
    
  case SDLGL_CAMERAMODE_TRANSLATE:
    if (gl_global.pressed_keys[PGKEY_w])
      glTranslatef(0.0,0.0,5.0*scale);
    if (gl_global.pressed_keys[PGKEY_s])
      glTranslatef(0.0,0.0,-5.0*scale);
    if (gl_global.pressed_keys[PGKEY_DOWN])
      glTranslatef(0.0,5.0*scale,0.0);
    if (gl_global.pressed_keys[PGKEY_UP])
      glTranslatef(0.0,-5.0*scale,0.0);
    if (gl_global.pressed_keys[PGKEY_RIGHT])
      glTranslatef(5.0*scale,0.0,0.0);
    if (gl_global.pressed_keys[PGKEY_LEFT])
      glTranslatef(-5.0*scale,0.0,0.0);
    break;

  case SDLGL_CAMERAMODE_ROTATE:
    if (gl_global.pressed_keys[PGKEY_w])
      glRotatef(0.4*scale,0.0,0.0,1.0);
    if (gl_global.pressed_keys[PGKEY_s])
      glRotatef(-0.4*scale,0.0,0.0,1.0);
    if (gl_global.pressed_keys[PGKEY_UP])
      glRotatef(0.4*scale,1.0,0.0,0.0);
    if (gl_global.pressed_keys[PGKEY_DOWN])
      glRotatef(-0.4*scale,1.0,0.0,0.0);
    if (gl_global.pressed_keys[PGKEY_LEFT])
      glRotatef(0.4*scale,0.0,1.0,0.0);
    if (gl_global.pressed_keys[PGKEY_RIGHT])
      glRotatef(-0.4*scale,0.0,1.0,0.0);
    break;
    
  }
}

/* Allow modifier keys to scale movements */
float gl_get_key_scale(void) {
  if (gl_global.pressed_keys[PGKEY_LSHIFT] || gl_global.pressed_keys[PGKEY_RSHIFT])
    return 0.1;
  if (gl_global.pressed_keys[PGKEY_LCTRL] || gl_global.pressed_keys[PGKEY_RCTRL])
    return 10.0;

  return 1;
}

/* The End */









